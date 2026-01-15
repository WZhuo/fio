#include "fio/diskutil.h"

#include <dirent.h>
#include <libgen.h>
#include <unistd.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <format>
#include <string_view>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef __linux__
#  include <sys/sysmacros.h>
#endif

#ifdef CONFIG_VALGRIND_DEV
#  include <valgrind/drd.h>
#else
#  define DRD_IGNORE_VAR(x) \
    do {                    \
    } while (0)
#endif

namespace fio {

// Static member definitions
int DiskUtil::last_majdev_ = -1;
int DiskUtil::last_mindev_ = -1;
std::shared_ptr<DiskUtil> DiskUtil::last_du_ = {};

DiskUtil::DiskUtil() { time_ = {}; }

DiskUtil::~DiskUtil() {}

void DiskUtil::set_path(std::string_view stat_path) {
  path_ = std::format("{}/stat", stat_path);
}

void DiskUtil::modify_users(int val) {
  lock_.Down();

  users_ += val;

  for (auto& slave : slaves_) {
    slave->users_ += val;
  }

  lock_.Up();
}

void DiskUtil::init_lock() {
  lock_.Init(static_cast<int32_t>(fio::Sem::State::UNLOCKED));
}

int DiskUtil::get_io_ticks(DiskUtilStat* dus) {
  char line[256];

  // dprint(FD_DISKUTIL, "open stat file: %s\n", path_);

  FILE* f = fopen(path_.data(), "r");
  if (!f) {
    return 1;
  }

  char* p = fgets(line, sizeof(line), f);
  if (!p) {
    fclose(f);
    return 1;
  }

  // dprint(FD_DISKUTIL, "%s: %s", path_, p);

  int ret =
      sscanf(p,
             "%" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
             " "
             "%" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
             " "
             "%*u %" SCNu64 " %" SCNu64 "\n",
             &dus->s.ios[0], &dus->s.merges[0], &dus->s.sectors[0], &dus->s.ticks[0],
             &dus->s.ios[1], &dus->s.merges[1], &dus->s.sectors[1], &dus->s.ticks[1],
             &dus->s.io_ticks, &dus->s.time_in_queue);
  fclose(f);
  // dprint(FD_DISKUTIL, "%s: stat read ok? %d\n", path_, ret == 10);
  return ret != 10;
}

namespace {

uint64_t safe_32bit_diff(uint64_t nval, uint64_t oval) {
  /*
   * Linux kernel prints some of the stat fields as 32-bit integers. It is
   * possible that the value overflows, but since fio uses unsigned 64-bit
   * arithmetic in update_io_tick_disk(), it instead results in a huge
   * bogus value being added to the respective accumulating field. Just
   * in case Linux starts reporting these metrics as 64-bit values in the
   * future, check that overflow actually happens around the 32-bit
   * unsigned boundary; assume overflow only happens once between
   * successive polls.
   */
  if (oval <= nval || oval >= (1ull << 32)) {
    return nval - oval;
  }
  return (1ull << 32) + nval - oval;
}

}  // namespace

void DiskUtil::update_io_tick() {
  if (!users_) {
    return;
  }

  DiskUtilStat temp_dus{};
  if (get_io_ticks(&temp_dus)) {
    return;
  }

  DiskUtilStat* dus = &dus_;
  DiskUtilStat* ldus = &last_dus_;

  dus->s.sectors[0] += (temp_dus.s.sectors[0] - ldus->s.sectors[0]);
  dus->s.sectors[1] += (temp_dus.s.sectors[1] - ldus->s.sectors[1]);
  dus->s.ios[0] += (temp_dus.s.ios[0] - ldus->s.ios[0]);
  dus->s.ios[1] += (temp_dus.s.ios[1] - ldus->s.ios[1]);
  dus->s.merges[0] += (temp_dus.s.merges[0] - ldus->s.merges[0]);
  dus->s.merges[1] += (temp_dus.s.merges[1] - ldus->s.merges[1]);
  dus->s.ticks[0] += safe_32bit_diff(temp_dus.s.ticks[0], ldus->s.ticks[0]);
  dus->s.ticks[1] += safe_32bit_diff(temp_dus.s.ticks[1], ldus->s.ticks[1]);
  dus->s.io_ticks += safe_32bit_diff(temp_dus.s.io_ticks, ldus->s.io_ticks);
  dus->s.time_in_queue +=
      safe_32bit_diff(temp_dus.s.time_in_queue, ldus->s.time_in_queue);

  timepoint t = std::chrono::steady_clock::now();
  dus->s.msec += std::chrono::duration_cast<std::chrono::milliseconds>(t - time_).count();
  time_ = t;
  ldus->s = temp_dus.s;
}

int DiskUtil::update_io_ticks() {
  // dprint(FD_DISKUTIL, "update io ticks\n");

  disk_util_sem_.Down();

  int ret = 0;
  // if (!helper_should_exit()) {
  if (true) {
    for (auto& entry : disk_list_) {
      entry->update_io_tick();
    }
  } else {
    ret = 1;
  }

  disk_util_sem_.Up();
  return ret;
}

std::shared_ptr<DiskUtil> DiskUtil::find(int major, int minor) {
  disk_util_sem_.Down();

  for (const auto& entry : disk_list_) {
    if (major == entry->major_ && minor == entry->minor_) {
      disk_util_sem_.Up();
      return entry;
    }
  }

  disk_util_sem_.Up();
  return nullptr;
}

Status DiskUtil::get_device_numbers(char* file_name, int* maj, int* min) {
  struct stat st;
  int majdev, mindev;
  char tempname[PATH_MAX];

  if (!lstat(file_name, &st)) {
    if (S_ISBLK(st.st_mode)) {
      majdev = major(st.st_rdev);
      mindev = minor(st.st_rdev);
    } else if (S_ISCHR(st.st_mode) || S_ISFIFO(st.st_mode)) {
      return InvalidArgument("not a block device");
    } else {
      majdev = major(st.st_dev);
      mindev = minor(st.st_dev);
    }
  } else {
    // Must be a file, open "." in that path
    snprintf(tempname, sizeof(tempname), "%s", file_name);
    char* p = dirname(tempname);
    if (stat(p, &st)) {
      perror("disk util stat");
      return InvalidArgument("must be a valid file or block device");
    }

    majdev = major(st.st_dev);
    mindev = minor(st.st_dev);
  }

  *min = mindev;
  *maj = majdev;

  return {};
}

Status DiskUtil::read_block_dev_entry(char* path, int* maj, int* min) {
  FILE* f = fopen(path, "r");
  if (!f) {
    perror("open path");
    return InvalidArgument("failed to open path");
  }

  char line[256];
  char* p = fgets(line, sizeof(line), f);
  fclose(f);

  if (!p) {
    return InvalidArgument("failed to read line");
  }

  if (sscanf(p, "%u:%u", maj, min) != 2) {
    return InvalidArgument("failed to parse major/minor numbers");
  }

  return {};
}

bool DiskUtil::check_dev_match(int majdev, int mindev, char* path) {
  int major_found, minor_found;

  if (read_block_dev_entry(path, &major_found, &minor_found)) {
    return false;
  }

  return (majdev == major_found && mindev == minor_found);
}

bool DiskUtil::find_block_dir(int majdev, int mindev, char* path, bool link_ok) {
  DIR* D = opendir(path);
  if (!D) {
    return false;
  }

  bool found = false;
  struct dirent* dir;
  while ((dir = readdir(D)) != nullptr) {
    char full_path[PATH_MAX];

    if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) {
      continue;
    }

    snprintf(full_path, sizeof(full_path), "%s/%s", path, dir->d_name);

    if (!strcmp(dir->d_name, "dev")) {
      if (check_dev_match(majdev, mindev, full_path)) {
        found = true;
        break;
      }
    }

    struct stat st;
    if (link_ok) {
      if (stat(full_path, &st) == -1) {
        perror("stat");
        break;
      }
    } else {
      if (lstat(full_path, &st) == -1) {
        perror("stat");
        break;
      }
    }

    if (!S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode)) {
      continue;
    }

    found = find_block_dir(majdev, mindev, full_path, false);
    if (found) {
      strcpy(path, full_path);
      break;
    }
  }

  closedir(D);
  return found;
}

void DiskUtil::find_add_slaves(thread_data* td, char* path,
                               std::shared_ptr<DiskUtil> masterdu) {
  char slavesdir[PATH_MAX];
  snprintf(slavesdir, sizeof(slavesdir), "%s/slaves", path);

  DIR* dirhandle = opendir(slavesdir);
  if (!dirhandle) {
    return;
  }

  struct dirent* dirent;
  while ((dirent = readdir(dirhandle)) != nullptr) {
    if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, "..")) {
      continue;
    }

    char temppath[PATH_MAX];
    char slavepath[PATH_MAX];
    snprintf(temppath, sizeof(temppath), "%s/%s", slavesdir, dirent->d_name);

    ssize_t linklen = readlink(temppath, slavepath, PATH_MAX - 1);
    if (linklen < 0) {
      perror("readlink() for slave device.");
      closedir(dirhandle);
      return;
    }
    slavepath[linklen] = '\0';

    snprintf(temppath, sizeof(temppath), "%s/%s/dev", slavesdir, slavepath);
    if (access(temppath, F_OK) != 0) {
      snprintf(temppath, sizeof(temppath), "%s/%s/device/dev", slavesdir, slavepath);
    }

    int majdev, mindev;
    if (read_block_dev_entry(temppath, &majdev, &mindev)) {
      perror("Error getting slave device numbers");
      closedir(dirhandle);
      return;
    }

    // See if this maj,min already exists
    auto slavedu = find(majdev, mindev);
    if (slavedu) {
      continue;
    }

    snprintf(temppath, sizeof(temppath), "%s/%s", slavesdir, slavepath);
    init_per_file_internal(td, majdev, mindev, temppath);
    slavedu = find(majdev, mindev);

    if (slavedu) {
      slavedu->users_++;
      slavedu->master_ = masterdu;
      masterdu->slaves_.push_back(slavedu);
    }
  }

  closedir(dirhandle);
}

std::shared_ptr<DiskUtil> DiskUtil::add(thread_data* td, int majdev, int mindev,
                                        char* path) {
  // dprint(FD_DISKUTIL, "add maj/min %d/%d: %s\n", majdev, mindev, path);

  auto du = create();

  DRD_IGNORE_VAR(du->users_);

  du->set_path(path);
  du->dus_.name = std::format("{}", basename(path));
  du->sysfs_root_ = strdup(path);
  du->major_ = majdev;
  du->minor_ = mindev;
  du->init_lock();
  du->users_ = 0;

  disk_util_sem_.Down();

  for (auto& entry : disk_list_) {
    // dprint(FD_DISKUTIL, "found %s in list\n", __du->dus_.name);

    if (du->dus_.name == entry->dus_.name) {
      disk_util_sem_.Up();
      return entry;
    }
  }

  // dprint(FD_DISKUTIL, "add %s to list\n", du->dus_.name);

  du->time_ = timepoint(std::chrono::steady_clock::now());
  du->get_io_ticks(&du->last_dus_);

  disk_list_.insert(disk_list_.end(), std::move(du));
  disk_util_sem_.Up();

  find_add_slaves(td, path, du);
  return du;
}

std::shared_ptr<DiskUtil> DiskUtil::init_per_file_internal(thread_data* td, int majdev,
                                                           int mindev, char* path) {
  struct stat st;
  char tmp[PATH_MAX];

  /*
   * If there's a ../queue/ directory there, we are inside a partition.
   * Check if that is the case and jump back. For loop/md/dm etc we
   * are already in the right spot.
   */
  snprintf(tmp, sizeof(tmp), "%s/../queue", path);
  if (!stat(tmp, &st)) {
    char* p = dirname(path);
    snprintf(tmp, sizeof(tmp), "%s/queue", p);
    if (stat(tmp, &st)) {
      // log_err("unknown sysfs layout\n");
      return nullptr;
    }
    snprintf(tmp, sizeof(tmp), "%s", p);
    snprintf(path, PATH_MAX, "%s", tmp);
  }

  return add(td, majdev, mindev, path);
}

std::shared_ptr<DiskUtil> DiskUtil::init_per_file(thread_data* td, char* filename) {
  int mindev, majdev;

  if (get_device_numbers(filename, &majdev, &mindev)) {
    return nullptr;
  }

  // dprint(FD_DISKUTIL, "%s belongs to maj/min %d/%d\n", filename, majdev, mindev);

  auto du = find(majdev, mindev);
  if (du) {
    return du;
  }

  /*
   * For an fs without a device, we will repeatedly stat through
   * sysfs which can take oodles of time for thousands of files. so
   * cache the last lookup and compare with that before going through
   * everything again.
   */
  if (mindev == last_mindev_ && majdev == last_majdev_) {
    return last_du_;
  }

  last_mindev_ = mindev;
  last_majdev_ = majdev;

  char foo[PATH_MAX];
  snprintf(foo, sizeof(foo), "/sys/block");
  if (!find_block_dir(majdev, mindev, foo, true)) {
    return nullptr;
  }

  last_du_ = init_per_file_internal(td, majdev, mindev, foo);
  return last_du_;
}

void DiskUtil::init_for_thread(thread_data* td) {
  // if (!td->o.do_disk_util || td_ioengine_flagged(td, FIO_DISKLESSIO | FIO_NODISKUTIL))
  // {
  //   return;
  // }

  // struct fio_file* f;
  // unsigned int i;
  // for_each_file(td, f, i) { f->du = init_per_file(td, f->file_name); }
}

void DiskUtil::prune_entries() {
  disk_util_sem_.Down();
  disk_list_.clear();

  last_majdev_ = last_mindev_ = -1;
  disk_util_sem_.Up();
}

std::shared_ptr<DiskUtil> DiskUtil::create() { return std::make_shared<DiskUtil>(); }

void DiskUtil::setup() {
  disk_util_sem_.Init(static_cast<int32_t>(Sem::State::UNLOCKED));
}

}  // namespace fio
