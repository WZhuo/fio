#pragma once

#include <deque>
#include <memory>
#include <string>
#include <string_view>

#include "fio/fio.h"
#include "fio/result.h"
#include "fio/sem.h"
#include "fio/time.h"

namespace fio {

inline constexpr size_t kDuNameSize = 64;
inline constexpr int kDiskUtilMills = 250;

/**
 * Disk utilization statistics
 *
 * @ios: Number of I/O operations that have been completed successfully.
 * @merges: Number of I/O operations that have been merged.
 * @sectors: I/O size in 512-byte units.
 * @ticks: Time spent on I/O in milliseconds.
 * @io_ticks: CPU time spent on I/O in milliseconds.
 * @time_in_queue: Weighted time spent doing I/O in milliseconds.
 *
 * For the array members, index 0 refers to reads and index 1 refers to writes.
 */
struct DiskUtilStats {
  std::array<uint64_t, 2> ios = {0, 0};
  std::array<uint64_t, 2> merges = {0, 0};
  std::array<uint64_t, 2> sectors = {0, 0};
  std::array<uint64_t, 2> ticks = {0, 0};
  uint64_t io_ticks = 0;
  uint64_t time_in_queue = 0;
  uint64_t msec = 0;
};

/**
 * Disk utilization as read from /sys/block/<dev>/stat
 */
struct DiskUtilStat {
  std::string name;
  DiskUtilStats s{};
};

/**
 * Aggregated disk utilization
 */
struct DiskUtilAgg {
  std::array<uint64_t, 2> ios = {0, 0};
  std::array<uint64_t, 2> merges = {0, 0};
  std::array<uint64_t, 2> sectors = {0, 0};
  std::array<uint64_t, 2> ticks = {0, 0};
  uint64_t io_ticks = 0;
  uint64_t time_in_queue = 0;
  uint32_t slavecount = 0;
  uint32_t pad = 0;
  // fio_fp64_t max_util{};
};

/**
 * Per-device disk utilization management class
 */
class DiskUtil {
 public:
  DiskUtil();
  ~DiskUtil();

  // Non-copyable
  DiskUtil(const DiskUtil&) = delete;
  DiskUtil& operator=(const DiskUtil&) = delete;

  // Non-movable (due to list membership)
  DiskUtil(DiskUtil&&) = delete;
  DiskUtil& operator=(DiskUtil&&) = delete;

  // Accessors
  [[nodiscard]] int major_dev() const noexcept { return major_; }
  [[nodiscard]] int minor_dev() const noexcept { return minor_; }
  [[nodiscard]] unsigned long users() const noexcept { return users_; }
  [[nodiscard]] const std::string& path() const noexcept { return path_; }
  [[nodiscard]] const std::string& sysfs_root() const noexcept { return sysfs_root_; }

  [[nodiscard]] const DiskUtilStat& dustat() const noexcept { return dus_; }
  [[nodiscard]] const DiskUtilStat& last_stat() const noexcept { return last_dus_; }
  [[nodiscard]] const DiskUtilAgg& agg() const noexcept { return agg_; }
  [[nodiscard]] const timepoint& time() const noexcept { return time_; }

  // Modifiers
  void set_major(int maj) noexcept { major_ = maj; }
  void set_minor(int min) noexcept { minor_ = min; }
  void set_sysfs_root(std::string_view root) noexcept { sysfs_root_ = root; }
  void set_path(std::string_view stat_path);

  /**
   * Modify user count by val for this device and all slaves
   */
  void modify_users(int val);
  void inc_users() { modify_users(1); }
  void dec_users() { modify_users(-1); }
  void add_user() noexcept { users_++; }
  void remove_user() noexcept {
    if (users_ > 0) users_--;
  }

  // Lock management
  void init_lock();
  void destroy_lock();
  [[nodiscard]] Sem* lock() noexcept { return &lock_; }

  [[nodiscard]] const std::shared_ptr<DiskUtil>& master() const noexcept {
    return master_;
  }
  [[nodiscard]] const std::deque<std::shared_ptr<DiskUtil>>& slaves() const noexcept {
    return slaves_;
  }

  // Static factory and management
  static std::shared_ptr<DiskUtil> create();

  // Global disk util management
  static void setup();
  static void prune_entries();
  static int update_io_ticks();
  static void init_for_thread(thread_data* td);

  // Lookup
  static std::shared_ptr<DiskUtil> find(int major, int minor);

 private:
  // Hook into master's list if this is a slave
  std::shared_ptr<DiskUtil> master_;

  /**
   * For software raids, maintains pointers to slave device entries.
   * Slave entries should be maintained through disk_list for memory
   * management. This list is only for aggregating RAID disk util figures.
   */
  std::deque<std::shared_ptr<DiskUtil>> slaves_;

  std::string sysfs_root_;
  std::string path_;
  int major_ = 0;
  int minor_ = 0;

  DiskUtilStat dus_{};
  DiskUtilStat last_dus_{};
  DiskUtilAgg agg_{};

  timepoint time_{};
  Sem lock_;
  unsigned long users_ = 0;

  // Private helpers
  static std::shared_ptr<DiskUtil> add(thread_data* td, int majdev, int mindev,
                                       char* path);
  static std::shared_ptr<DiskUtil> init_per_file(thread_data* td, char* filename);
  static std::shared_ptr<DiskUtil> init_per_file_internal(thread_data* td, int majdev,
                                                          int mindev, char* path);
  static void find_add_slaves(thread_data* td, char* path,
                              std::shared_ptr<DiskUtil> master);
  static Status get_device_numbers(char* filename, int* maj, int* min);
  static Status read_block_dev_entry(char* path, int* maj, int* min);
  static bool find_block_dir(int majdev, int mindev, char* path, bool link_ok);
  static bool check_dev_match(int majdev, int mindev, char* path);

  int get_io_ticks(DiskUtilStat* dus);
  void update_io_tick();

  // Static state
  static Sem disk_util_sem_;
  static std::deque<std::shared_ptr<DiskUtil>> disk_list_;
  static int last_majdev_;
  static int last_mindev_;
  static std::shared_ptr<DiskUtil> last_du_;
};

}  // namespace fio
