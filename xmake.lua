-- xmake.lua
set_project("Personalized")
set_version("1.0.0")

set_languages("c++20")

target("Personalized")
    set_kind("shared")
    add_files("*.cpp")
    
    -- This defines a bypass macro so the missing LeviLamina files aren't needed
    add_defines("DISABLE_LL_LOGGER")
    
    add_includedirs(".")
