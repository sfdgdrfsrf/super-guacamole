-- xmake.lua
set_project("Personalized")
set_version("1.0.0")

set_languages("c++20")

target("Personalized")
    set_kind("shared")
    add_files("*.cpp")
    
    -- This points the compiler to the current folder for any path style
    add_includedirs(".")
    add_includedirs("./Utils")
    add_includedirs("./Hooks")
