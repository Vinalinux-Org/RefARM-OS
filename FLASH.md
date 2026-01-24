# Hướng dẫn Flash Image vào SD Card

## Chuẩn bị

### Hardware
- SD card (tối thiểu 512MB)
- Card reader
- BeagleBone Black
- Cáp UART (USB-to-TTL)

### Software
```bash
sudo apt update
sudo apt install -y gcc-arm-linux-gnueabihf u-boot-tools screen
```

## Quy trình Flash

### Bước 1: Xác định SD card

```bash
# Cắm SD card vào máy tính
lsblk

# Output:
# sda    500G  disk  <- Ổ hệ thống (ĐỪNG ĐỘNG!)
# sdb      8G  disk  <- SD card của bạn
```

**Chú ý:** Thay `/dev/sdb` bằng device thực tế!

### Bước 2: Setup SD card (chỉ làm 1 lần)

```bash
cd scripts
./setup_sdcard.sh /dev/sdb

# Gõ: yes
```

### Bước 3: Build và Deploy

```bash
# Vẫn ở thư mục scripts
./deploy.sh /dev/sdb
```

Script tự động:
- Build kernel
- Tạo uImage
- Copy MLO, u-boot.img, uEnv.txt, uImage vào SD card

### Bước 4: Boot trên BeagleBone Black

1. Rút SD card an toàn
2. Cắm SD card vào BBB
3. Kết nối UART:
   - GND (Pin 1) → GND
   - RX  (Pin 4) → TX của UART
   - TX  (Pin 5) → RX của UART
4. Mở serial console:
   ```bash
   ./serial_console.sh
   ```
   Hoặc MobaXterm: Serial, 115200, COM3
5. **Giữ nút BOOT** (gần slot SD) và cắm nguồn
6. Nhả nút sau 2-3 giây

## Output kỳ vọng

```
U-Boot SPL ...
U-Boot ...
Booting RefARM-OS...
Starting kernel ...

====================================
 AM335x OS Reference Kernel Started
====================================
Kernel is alive.
```

## Rebuild (sau khi sửa code)

```bash
# Chỉ cần chạy lại deploy
cd scripts
./deploy.sh /dev/sdb
```

## Troubleshooting

**Lỗi: Device not found**
```bash
# Kiểm tra lại device
lsblk
```

**Lỗi: Permission denied**
```bash
# Chạy script với sudo hoặc thêm user vào group
sudo usermod -a -G disk $USER
# Logout và login lại
```

**Kernel không boot**
- Kiểm tra đã giữ nút BOOT khi cắm nguồn chưa
- Kiểm tra files trên SD card: `ls /media/$USER/BOOT/`
- Phải có: MLO, u-boot.img, uImage, uEnv.txt