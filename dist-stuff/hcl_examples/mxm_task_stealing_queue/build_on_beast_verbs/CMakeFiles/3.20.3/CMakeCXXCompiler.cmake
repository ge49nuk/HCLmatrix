set(CMAKE_CXX_COMPILER "/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/mpich-3.3.2-ooqbcd3h5ph56ysycubcg56i6rq72gqz/bin/mpicxx")
set(CMAKE_CXX_COMPILER_ARG1 "")
set(CMAKE_CXX_COMPILER_ID "GNU")
set(CMAKE_CXX_COMPILER_VERSION "10.2.1")
set(CMAKE_CXX_COMPILER_VERSION_INTERNAL "")
set(CMAKE_CXX_COMPILER_WRAPPER "")
set(CMAKE_CXX_STANDARD_COMPUTED_DEFAULT "14")
set(CMAKE_CXX_COMPILE_FEATURES "cxx_std_98;cxx_template_template_parameters;cxx_std_11;cxx_alias_templates;cxx_alignas;cxx_alignof;cxx_attributes;cxx_auto_type;cxx_constexpr;cxx_decltype;cxx_decltype_incomplete_return_types;cxx_default_function_template_args;cxx_defaulted_functions;cxx_defaulted_move_initializers;cxx_delegating_constructors;cxx_deleted_functions;cxx_enum_forward_declarations;cxx_explicit_conversions;cxx_extended_friend_declarations;cxx_extern_templates;cxx_final;cxx_func_identifier;cxx_generalized_initializers;cxx_inheriting_constructors;cxx_inline_namespaces;cxx_lambdas;cxx_local_type_template_args;cxx_long_long_type;cxx_noexcept;cxx_nonstatic_member_init;cxx_nullptr;cxx_override;cxx_range_for;cxx_raw_string_literals;cxx_reference_qualified_functions;cxx_right_angle_brackets;cxx_rvalue_references;cxx_sizeof_member;cxx_static_assert;cxx_strong_enums;cxx_thread_local;cxx_trailing_return_types;cxx_unicode_literals;cxx_uniform_initialization;cxx_unrestricted_unions;cxx_user_literals;cxx_variadic_macros;cxx_variadic_templates;cxx_std_14;cxx_aggregate_default_initializers;cxx_attribute_deprecated;cxx_binary_literals;cxx_contextual_conversions;cxx_decltype_auto;cxx_digit_separators;cxx_generic_lambdas;cxx_lambda_init_captures;cxx_relaxed_constexpr;cxx_return_type_deduction;cxx_variable_templates;cxx_std_17;cxx_std_20")
set(CMAKE_CXX98_COMPILE_FEATURES "cxx_std_98;cxx_template_template_parameters")
set(CMAKE_CXX11_COMPILE_FEATURES "cxx_std_11;cxx_alias_templates;cxx_alignas;cxx_alignof;cxx_attributes;cxx_auto_type;cxx_constexpr;cxx_decltype;cxx_decltype_incomplete_return_types;cxx_default_function_template_args;cxx_defaulted_functions;cxx_defaulted_move_initializers;cxx_delegating_constructors;cxx_deleted_functions;cxx_enum_forward_declarations;cxx_explicit_conversions;cxx_extended_friend_declarations;cxx_extern_templates;cxx_final;cxx_func_identifier;cxx_generalized_initializers;cxx_inheriting_constructors;cxx_inline_namespaces;cxx_lambdas;cxx_local_type_template_args;cxx_long_long_type;cxx_noexcept;cxx_nonstatic_member_init;cxx_nullptr;cxx_override;cxx_range_for;cxx_raw_string_literals;cxx_reference_qualified_functions;cxx_right_angle_brackets;cxx_rvalue_references;cxx_sizeof_member;cxx_static_assert;cxx_strong_enums;cxx_thread_local;cxx_trailing_return_types;cxx_unicode_literals;cxx_uniform_initialization;cxx_unrestricted_unions;cxx_user_literals;cxx_variadic_macros;cxx_variadic_templates")
set(CMAKE_CXX14_COMPILE_FEATURES "cxx_std_14;cxx_aggregate_default_initializers;cxx_attribute_deprecated;cxx_binary_literals;cxx_contextual_conversions;cxx_decltype_auto;cxx_digit_separators;cxx_generic_lambdas;cxx_lambda_init_captures;cxx_relaxed_constexpr;cxx_return_type_deduction;cxx_variable_templates")
set(CMAKE_CXX17_COMPILE_FEATURES "cxx_std_17")
set(CMAKE_CXX20_COMPILE_FEATURES "cxx_std_20")
set(CMAKE_CXX23_COMPILE_FEATURES "")

set(CMAKE_CXX_PLATFORM_ID "Linux")
set(CMAKE_CXX_SIMULATE_ID "")
set(CMAKE_CXX_COMPILER_FRONTEND_VARIANT "")
set(CMAKE_CXX_SIMULATE_VERSION "")




set(CMAKE_AR "/usr/bin/ar")
set(CMAKE_CXX_COMPILER_AR "/usr/bin/gcc-ar-10")
set(CMAKE_RANLIB "/usr/bin/ranlib")
set(CMAKE_CXX_COMPILER_RANLIB "/usr/bin/gcc-ranlib-10")
set(CMAKE_LINKER "/usr/bin/ld")
set(CMAKE_MT "")
set(CMAKE_COMPILER_IS_GNUCXX 1)
set(CMAKE_CXX_COMPILER_LOADED 1)
set(CMAKE_CXX_COMPILER_WORKS TRUE)
set(CMAKE_CXX_ABI_COMPILED TRUE)
set(CMAKE_COMPILER_IS_MINGW )
set(CMAKE_COMPILER_IS_CYGWIN )
if(CMAKE_COMPILER_IS_CYGWIN)
  set(CYGWIN 1)
  set(UNIX 1)
endif()

set(CMAKE_CXX_COMPILER_ENV_VAR "CXX")

if(CMAKE_COMPILER_IS_MINGW)
  set(MINGW 1)
endif()
set(CMAKE_CXX_COMPILER_ID_RUN 1)
set(CMAKE_CXX_SOURCE_FILE_EXTENSIONS C;M;c++;cc;cpp;cxx;m;mm;mpp;CPP)
set(CMAKE_CXX_IGNORE_EXTENSIONS inl;h;hpp;HPP;H;o;O;obj;OBJ;def;DEF;rc;RC)

foreach (lang C OBJC OBJCXX)
  if (CMAKE_${lang}_COMPILER_ID_RUN)
    foreach(extension IN LISTS CMAKE_${lang}_SOURCE_FILE_EXTENSIONS)
      list(REMOVE_ITEM CMAKE_CXX_SOURCE_FILE_EXTENSIONS ${extension})
    endforeach()
  endif()
endforeach()

set(CMAKE_CXX_LINKER_PREFERENCE 30)
set(CMAKE_CXX_LINKER_PREFERENCE_PROPAGATES 1)

# Save compiler ABI information.
set(CMAKE_CXX_SIZEOF_DATA_PTR "8")
set(CMAKE_CXX_COMPILER_ABI "ELF")
set(CMAKE_CXX_BYTE_ORDER "LITTLE_ENDIAN")
set(CMAKE_CXX_LIBRARY_ARCHITECTURE "")

if(CMAKE_CXX_SIZEOF_DATA_PTR)
  set(CMAKE_SIZEOF_VOID_P "${CMAKE_CXX_SIZEOF_DATA_PTR}")
endif()

if(CMAKE_CXX_COMPILER_ABI)
  set(CMAKE_INTERNAL_PLATFORM_ABI "${CMAKE_CXX_COMPILER_ABI}")
endif()

if(CMAKE_CXX_LIBRARY_ARCHITECTURE)
  set(CMAKE_LIBRARY_ARCHITECTURE "")
endif()

set(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX "")
if(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX)
  set(CMAKE_CL_SHOWINCLUDES_PREFIX "${CMAKE_CXX_CL_SHOWINCLUDES_PREFIX}")
endif()





set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/mpich-3.3.2-ooqbcd3h5ph56ysycubcg56i6rq72gqz/include;/home/ge49nuk2/localLibs/hclInstall/include;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/mochi-thallium-0.7-mct2gwtys3oioeyqku5aetkbornvbmqf/include;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/mochi-margo-0.9.4-wi5xj2h72rnqyfe4nmkqwwnjnh3pzlfp/include;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/mochi-abt-io-0.5.1-cvewwn6mu3phyrw5o7shoegihiczwynj/include;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/mercury-2.0.1-5llxbgdp7uxw5hiqwsh6oqmhidktlisz/include;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/libfabric-1.11.1-iy4glnm7smozwwqtfs5cy2zko47h65mk/include;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/cereal-1.3.0-vd6dtp3ytdngo7ivuogbzlmq54zsm2kk/include;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/boost-1.76.0-hotyooiavzkc6kwweehdsc3jwyiepnig/include;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/argobots-1.1-jlfvqv6uxajfrabrdf5ymghaoe6jf56j/include;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/hwloc-2.4.1-bu232a3groh66oalha3d44er3olqd6yh/include;/usr/include/c++/10;/usr/include/c++/10/x86_64-suse-linux;/usr/include/c++/10/backward;/usr/lib64/gcc/x86_64-suse-linux/10/include;/usr/local/include;/usr/lib64/gcc/x86_64-suse-linux/10/include-fixed;/usr/x86_64-suse-linux/include;/usr/include")
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "mpicxx;mpi;stdc++;m;gcc_s;gcc;c;gcc_s;gcc")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/hwloc-2.4.1-bu232a3groh66oalha3d44er3olqd6yh/lib;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/mpich-3.3.2-ooqbcd3h5ph56ysycubcg56i6rq72gqz/lib;/home/ge49nuk2/localLibs/hclInstall/lib64;/usr/lib64/gcc/x86_64-suse-linux/10;/usr/lib64;/lib64;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/mochi-thallium-0.7-mct2gwtys3oioeyqku5aetkbornvbmqf/lib;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/mochi-margo-0.9.4-wi5xj2h72rnqyfe4nmkqwwnjnh3pzlfp/lib;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/mochi-abt-io-0.5.1-cvewwn6mu3phyrw5o7shoegihiczwynj/lib;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/mercury-2.0.1-5llxbgdp7uxw5hiqwsh6oqmhidktlisz/lib;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/libfabric-1.11.1-iy4glnm7smozwwqtfs5cy2zko47h65mk/lib;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/boost-1.76.0-hotyooiavzkc6kwweehdsc3jwyiepnig/lib;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/argobots-1.1-jlfvqv6uxajfrabrdf5ymghaoe6jf56j/lib;/home/ge49nuk2/spack/opt/spack/linux-sles15-zen2/gcc-10.2.1/intel-mpi-2019.10.317-n23qoovifkbmdnkh2rlhrawr4r6t5m24/compilers_and_libraries_2020.4.317/linux/mpi/intel64/libfabric/lib;/usr/x86_64-suse-linux/lib")
set(CMAKE_CXX_IMPLICIT_LINK_FRAMEWORK_DIRECTORIES "")