set_project("todo")
set_version("0.1.0")

set_languages("c++23")

add_rules("mode.debug", "mode.release")

add_requires("sqlite3")
add_requires("libbcrypt")
add_requires("qt6widgets")

target("todo")
set_kind("binary")
set_rundir("$(projectdir)")

add_rules("qt.widgetapp")

add_files("src/*.cpp")
add_files("include/ui.h")
add_includedirs("include")

add_packages("sqlite3")
add_packages("libbcrypt")
add_packages("qt6widgets")
