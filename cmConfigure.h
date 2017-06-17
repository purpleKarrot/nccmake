/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmConfigure_h
#define cmConfigure_h

#define CM_DISABLE_COPY(Class)                                                \
  Class(Class const&) = delete;                                               \
  Class& operator=(Class const&) = delete;

#define CM_NULLPTR nullptr

#define CM_OVERRIDE override

#define CMAKE_STANDARD_OPTIONS_TABLE                                          \
  {                                                                           \
    nullptr, nullptr                                                          \
  }

#endif
