# 样例总览

本目录按样例域维护文档入口：

- [motor.md](motor.md)
  电机/执行机构链路验证样例
- [communication.md](communication.md)
  ARES 通信与 Plotter 协议样例
- [imu.md](imu.md)
  IMU 姿态与加速度计标定样例
- [chassis.md](chassis.md)
  以 SBUS 控制为入口的底盘样例
- [vcan.md](vcan.md)
  双通道 SPI-CAN 桥接样例

样例目录即构建入口名：

- `samples/motor/*`
- `samples/communication/*`
- `samples/IMU/*`
- `samples/chassis/`
- `samples/vcan/*`

通用构建命令：

```bash
west build -b <board> <sample_dir>
west flash
```

建议高频验证命令使用独立构建目录，避免 overlay 与编译缓存互相污染。

每类文档遵循固定字段：用途、构建命令、适用 board、硬件依赖、维护规则，作为样例签名页。
