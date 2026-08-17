-- xmake.lua
set_project("Personalized")
set_version("1.0.0")

set_languages("c++20")

-- Automatically download the modern JSON library from the cloud
add_requires("nlohmann_json")

target("Personalized")
    set_kind("shared")
    add_files("*.cpp")
    
    -- Link the downloaded JSON package directly into your mod binary
    add_packages("nlohmann_json")
    
    add_includedirs(".")
    add_includedirs("/tmp/fake_ll")
