#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <string_view>

namespace esphome {
namespace wordclock {

/**
 * @brief String pool to reduce memory duplication
 * 
 * Words are stored only once in the pool.
 * Maps use size_t indices instead of strings.
 * Estimated savings: ~35-40KB RAM.
 */
class StringPool {
 public:
  /**
   * @brief Singleton instance
   */
  static StringPool& instance() {
    static StringPool pool;
    return pool;
  }
  
  /**
   * @brief Interns a string and returns its index
   * @param str String to intern
   * @return Unique index of the string in the pool
   */
  size_t intern(const std::string& str) {
    auto it = index_.find(str);
    if (it != index_.end()) {
      return it->second;
    }
    
    size_t idx = pool_.size();
    pool_.push_back(str);
    index_[pool_.back()] = idx;
    return idx;
  }
  
  /**
   * @brief Retrieves a string by its index
   * @param idx Index of the string
   * @return Reference to the string or empty string if invalid index
   */
  const std::string& get(size_t idx) const {
    static const std::string empty;
    if (idx < pool_.size()) {
      return pool_[idx];
    }
    return empty;
  }
  
  /**
   * @brief Clears the pool (used on language change)
   */
  void clear() {
    pool_.clear();
    index_.clear();
  }
  
  /**
   * @brief Current pool size
   */
  size_t size() const { return pool_.size(); }
  
 private:
  StringPool() = default;
  StringPool(const StringPool&) = delete;
  StringPool& operator=(const StringPool&) = delete;
  
  std::vector<std::string> pool_;
  std::unordered_map<std::string, size_t> index_;
};

}  // namespace wordclock
}  // namespace esphome
