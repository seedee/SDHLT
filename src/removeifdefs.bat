echo off
set definition=%1
F:/Users/Sam/Downloads/sunifdef -R -D%definition% -Fc,cpp,h "zhlt-vluzacn"
git add "zhlt-vluzacn"
git commit -m "Remove %definition% ifdefs"