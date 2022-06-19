---@diagnostic disable: undefined-global


newaction {
   trigger = "clean",
   description = "clean build files",
   execute = function ()
      print("Cleaing build files...")
      local dirs = os.matchdirs("**")
      for k, v in pairs(dirs) do
         if string.contains(v,"build") then
            os.rmdir(v)
         end
      end
      print("all build files have been cleaned")
   end
}

if string.find("gmake gmake2 vs2005 vs2008 vs2010 vs2012 vs2013 vs2015 vs2017 vs2019 vs2022 xcode4", _ACTION) == nil then do return end end

BuidName = ""

--copy folder and all its content from src to dst 
function CopyFolder(src,dst)
   local fm = os.matchfiles( src .. "/**")
   local hm = os.matchdirs(src .."/**")
   --os.mkdir("path")
   for i, v in pairs(hm) do os.mkdir(dst .. "/".. v )  end
   for i, v in pairs(fm) do os.copyfile(v, dst .. "/" .. v) end
end

-- produce numbred builds in case of reproduction
BuidName = _ACTION .. "-build"
local hm = os.matchdirs(_ACTION.."-build**")
if rawlen(hm) > 0 then
   BuidName = _ACTION .. "-build" .. tostring(rawlen(hm))
end
CopyFolder("src",BuidName)

--if string.find("gmake gmake2",_ACTION) ~= nil then os.copyfile("compile_and_run.bat", BuidName .. "/compile_and_run.bat") end

workspace "T-Shphere"
   configurations {"Debug","Release"}
   architecture "x64"
   system("windows")
   filename "GLFW Startup"
   location (BuidName)

project "T-Shphere"
   kind "ConsoleApp" --WindowedApp
   language "C++"
   cppdialect "C++11"
   targetdir ( BuidName .. "/bin/%{cfg.buildcfg}/%{prj.name}")
   objdir (BuidName .."/ints")

   files {BuidName .."/src/**.h", BuidName .."/src/**.cpp"}
   libdirs { BuidName .."/src/outsrc/GLFW/lib"}
   links{ "glfw3","opengl32"}
   
   filter "action:gmake*"
      -- omdlg32, ole32 needed for tinyfolder. gdi32 needed for glfw static linkage
      links{ "comdlg32","ole32","gdi32" }
      -- use static runtime library on GCC
      buildoptions "-static-libstdc++"

   filter "configurations:Debug"
      runtime "Debug"
      defines {"Debug"}
      optimize "On"
      symbols "On"

   filter "configurations:Release"
      runtime "Release"
      defines {"NDEBUG"}
      optimize "On"
      symbols "Off"

   -- use static runtime library on visual studio builds
   filter { "action:vs*", "configurations:Debug" }
      buildoptions "/MTd"
      
   filter { "action:vs*", "configurations:Release" }
      buildoptions "/MT"

   filter "system:Windows"
      systemversion "latest" -- To use the latest version of the SDK available
