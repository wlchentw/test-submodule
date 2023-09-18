@@echo off 
FOR /F  %%T in ('findstr "." p') do (
 set LogPath=%%T
 )
