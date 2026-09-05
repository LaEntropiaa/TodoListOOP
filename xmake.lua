set_project("todo")
set_version("0.1.0")

set_languages("c++23")

add_rules("mode.debug", "mode.release")

add_requires("sqlite3")
add_requires("ftxui")

target("todo")
set_kind("binary")

add_files("src/*.cpp")
add_includedirs("include")

add_packages("sqlite3")
add_packages("ftxui")
