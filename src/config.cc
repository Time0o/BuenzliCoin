#include <string>
#include <string_view>
#include <type_traits>

#include <toml.hpp>

#include "clock.h"
#include "config.h"

namespace
{

void toml_for_table(auto const &table_parent,
                    std::string_view table_name,
                    auto &&action)
{
  if (!table_parent.contains(table_name))
    return;

  auto const &tbl { table_parent[table_name] };

  if (tbl.is_table())
    action(*tbl.as_table());
};

template<typename T>
void toml_assign(auto &var,
                 auto const &value_parent,
                 std::string_view value_name)
{
  if (!value_parent.contains(value_name))
    return;

  auto val { value_parent[value_name].template value<T>() };

  if (val)
    var = std::remove_reference_t<decltype(var)> { *val };
};

} // end namespace

namespace bc
{

Config Config::from_toml(std::string const &filename)
{
  Config cfg;

  auto t { toml::parse_file(filename) };

  toml_for_table(t, "blockgen", [&cfg](auto const &t) {
    toml_assign<clock::TimeInterval::rep>(
      cfg.blockgen_time_expected, t,
      "time_expected");
    toml_assign<clock::TimeInterval::rep>(
      cfg.blockgen_time_max_delta, t,
      "time_max_delta");
    toml_assign<double>(
      cfg.blockgen_difficulty_init, t,
      "difficulty_init");
    toml_assign<std::size_t>(
      cfg.blockgen_difficulty_adjust_after, t,
      "difficulty_adjust_after");
    toml_assign<double>(
      cfg.blockgen_difficulty_adjust_factor_limit, t,
      "difficulty_adjust_factor_limit");
  });

  toml_for_table(t, "transaction", [&cfg](auto const &t) {
    toml_assign<std::size_t>(
      cfg.transaction_num_per_block, t,
      "num_per_block");
    toml_assign<std::size_t>(
      cfg.transaction_reward_amount, t,
      "reward_amount");
  });

  return cfg;
}

} // end namespace bc
