::                         多语言批处理文件

echo off

cd ..

echo.
echo.
echo =================================================================
echo =                     多语言批处理程序                          =
echo =================================================================
echo.
echo.

del lang.pro
echo.
echo.

echo 1、更新或生成语言工程文件lang.pro
qmake -project -o lang.pro
echo.
echo.

echo 2、编辑语言工程文件lang.pro
echo CODECFORTR = System >> lang.pro
echo CODECFORSRC = System >> lang.pro
echo.
echo.

echo 3、更新语言翻译ts文件
pause
lupdate lang.pro
echo.
echo.

echo 4、编辑语言翻译ts文件
pause 
linguist TSResource/lang_en.ts TSResource/lang_zh_CN.ts 
echo.
echo.

echo 5、编译语言翻译ts文件生成qm文件
pause
lrelease lang.pro
echo.
echo.

echo 6、拷贝qm文件到language目录下
pause
cd TSResource
move *.qm  ../Resources/qm
cd ..
echo.
echo.

del lang.pro

echo 操作完成
echo.
echo.

pause

