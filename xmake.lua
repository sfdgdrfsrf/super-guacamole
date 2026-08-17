-- xmake.lua
set_project("Personalized")
set_version("1.0.0")

set_languages("c++20")

target("Personalized")
    set_kind("shared")
    add_files("*.cpp")
    
    -- This forces the compiler to look in the flat folder and our custom external temp folder
    add_includedirs(".")
    add_includedirs("/tmp/fake_ll")
