@REM # 1. 进入项目源码根目录
@REM cd /d D:\Desktop\yc\PartTimeJobs\Windows\CanController\CanController

@REM # 2. 删除旧Debug构建目录（必须清，否则类型不变）
rmdir /s /q ..\build

@REM # 3. 重新cmake，指定Release构建类型
cmake -B ..\build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

@REM # 4. 编译Release程序
cmake --build ..\build