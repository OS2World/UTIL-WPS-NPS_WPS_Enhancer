/* npswpsin.cmd - installer of NPS WPS Enhancer */

options etmode
options exmode
call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
call SysLoadFuncs

programName = SysSearchPath('PATH', 'NPSWPS.EXE')
if programName = '' then
do
   say 'Error: Cannot find NPSWPS.EXE in PATH directories.'
   exit
end

if stream('NPSWPS.DLL', 'command', 'query exists') = '' then
do
   say 'Error: Cannot find NPSWPS.DLL in the current directory.'
   exit
end

if SysCreateObject('WPProgram', 'NPS WPS Enhancer', '<WP_START>', 'EXENAME='programName';STARTUPDIR='directory()';PROGTYPE=PM;CCVIEW=YES;OBJECTID=<N.P.S.NPS WPS Enhancer>', 'u') = 0 then
   say 'Error: Cannot create object.'
else
   say 'The program object was successfully created in the start up folder.'

exit