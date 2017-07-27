//
// Created by Saliya Ekanayake on 7/17/17.
//

#ifndef CLIONCPP_UTILS_HPP
#define CLIONCPP_UTILS_HPP

#endif //CLIONCPP_UTILS_HPP

void to_cout(const std::vector<std::string> &v)
{
  std::copy(v.begin(), v.end(), std::ostream_iterator<std::string>{
    std::cout, "\n"});
}
