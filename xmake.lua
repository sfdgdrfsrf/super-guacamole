-- xmake.lua
set_project("Personalized")
set_version("1.0.0")

-- Configure for C++20 standard
set_languages("c++20")

target("Personalized")
    set_kind("shared")
    add_files("src/*.cpp")
    add_files("src/Hooks/*.cpp")
    add_files("src/Utils/*.cpp")
    
    -- Ensure include paths match standard project layouts
    add_includedirs("src")
    add_includedirs("src/Hooks")
    add_includedirs("src/Utils")
    -- Typical layout:
    --   LeviLamina/
    --     include/   ← ll/ mc/ headers
    --     lib/       ← prebuilt .lib files
    --
    -- If using the LeviLamina template repo, these are auto-configured.
    add_includedirs("$(projectdir)/LeviLamina/include", {public = true})
    add_includedirs("$(projectdir)/BDS/include", {public = true})

    -- ── Link libraries ──
    add_links("LeviLamina")

    -- ── System libraries ──
    if is_plat("windows") then
        add_syslinks("kernel32", "user32", "ntdll")
    end()

    -- ── Compiler flags ──
    -- MSVC-specific (BDS is Windows-only for native mods)
    if is_plat("windows") then
        add_cxxflags("/EHsc", {force = true})       -- C++ exceptions
        add_cxxflags("/W3", {force = true})          -- Warning level 3
        add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN")
    end()

    -- Debug-specific
    if is_mode("debug") then
        add_defines("DEBUG", "_DEBUG")
        add_cxxflags("/Zi", {force = true})          -- Debug info
        if is_plat("windows") then
            add_cxxflags("/Od", {force = true})      -- No optimization
        end()
    end()

    -- Release-specific
    if is_mode("release") then
        add_defines("NDEBUG")
        if is_plat("windows") then
            add_cxxflags("/O2", "/GL", {force = true})  -- Full optimization + LTCG
            add_ldflags("/LTCG", {force = true})
        end()
    end()

    -- ── Output configuration ──
    -- The DLL should end up in the plugins/ directory for LeviLamina
    after_build(function (target)
        import("core.base.option")
        local output = target:targetdir() .. "/Personalized.dll"
        print("Built: " .. output)
        print("Copy to: <server>/plugins/Personalized/Personalized.dll")
    end)

-- ─────────────────────────────────────────────
--  Optional: test target
-- ─────────────────────────────────────────────
target("PersonalizedTest")
    set_kind("binary")
    set_default(false)

    add_files("src/Utils/RandomMapper.cpp")
    add_files("tests/TestRandomMapper.cpp")

    add_includedirs("src")

    if is_plat("windows") then
        add_cxxflags("/EHsc", {force = true})
    end()
