-- xmake.lua
set_project("Personalized")
set_version("1.0.0")

set_languages("c++20")

target("Personalized")
    set_kind("shared")
    add_files("*.cpp")
    add_includedirs(".")
