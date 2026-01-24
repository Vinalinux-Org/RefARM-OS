# SD Card Layout and Boot Deployment

Tài liệu này mô tả cách chuẩn bị thẻ SD để boot
OS reference trên nền AM335x (BeagleBone Black),
sử dụng MLO và U-Boot do vendor cung cấp (tạm thời).

Mục tiêu của tài liệu:
- Giúp flash SD card đúng cách để board boot được kernel
- Làm rõ vai trò của từng file trên SD card
- Hỗ trợ debug khi boot thất bại

Tài liệu này mô tả **triển khai hiện tại**.
Khi bootloader được tự phát triển trong tương lai,
quy trình có thể thay đổi nhưng tư duy tổng thể vẫn giữ nguyên.

---

## 1. Tổng quan boot từ SD card trên AM335x

1.1. Khi cấp nguồn, Boot ROM trong SoC sẽ tìm thiết bị boot
theo thứ tự ưu tiên đã được cấu hình (boot pins).

1.2. Với boot từ SD card:
- Boot ROM đọc bảng định dạng phân vùng (Partition Table)
- Tìm file có tên chính xác là "MLO" trong phân vùng Boot
- Load MLO vào bộ nhớ đệm nội (SRAM) và thực thi

1.3. **Lưu ý về Boot ROM và Filesystem:**
- Boot ROM có hỗ trợ đọc FAT12/16/32.
- Tuy nhiên, trình đọc FAT của ROM rất đơn giản. Nó yêu cầu dữ liệu của file MLO phải nằm ở các **cluster đầu tiên** và liên tục.
- Đây là lý do bắt buộc phải copy MLO vào thẻ nhớ trống trước các file khác.

---

## 2. Yêu cầu thẻ SD

2.1. Loại thẻ:
- SD hoặc microSD
- Dung lượng nhỏ (≤ 32GB) khuyến nghị để tránh vấn đề tương thích

2.2. Phân vùng:
- Chỉ cần **1 phân vùng duy nhất**
- Kiểu: FAT32 (vfat)
- Boot flag: Khuyến nghị bật để đảm bảo Boot ROM nhận diện đúng phân vùng.

---

## 3. Cấu trúc file trên SD card

Cấu trúc tối thiểu của phân vùng FAT (Thứ tự copy là quan trọng):

```text
SD (FAT32)
├── MLO          <-- (1) Copy đầu tiên!
├── u-boot.img   <-- (2) Bootloader chính
├── uEnv.txt     <-- (3) Config boot (thay thế boot.scr)
└── kernel.bin   <-- (4) OS Kernel
```

### 3.1. MLO (SPL)
* **Vai trò:** First-stage bootloader (Secondary Program Loader).
* **Nguồn:** Do vendor cung cấp (tạm thời) hoặc build từ U-Boot SPL.
* **Cơ chế:** Được Boot ROM load trực tiếp từ SD card vào SRAM (64KB).
* **Yêu cầu Deploy (QUAN TRỌNG):**
    * Phải copy vào SD card **ĐẦU TIÊN** sau khi format.
    * Không được đổi tên (tên file bắt buộc phải là `MLO`).

### 3.2. u-boot.img
* **Vai trò:** Second-stage bootloader.
* **Cơ chế:** Được MLO load vào DDR RAM.
* **Chức năng:** Khởi tạo toàn bộ phần cứng, cung cấp giao diện dòng lệnh (console) và nạp Kernel.

### 3.3. uEnv.txt
* **Vai trò:** File cấu hình môi trường dạng text cho U-Boot.
* **Cơ chế:** U-Boot tự động đọc file này khi khởi động để ghi đè các biến môi trường mặc định.
* **Nội dung mẫu (cho Raw Binary Kernel):**

```bash
# Cấu hình Serial Console
console=ttyO0,115200n8

# Địa chỉ load kernel vào RAM (DDR)
loadaddr=0x80000000

# Tên file kernel (OS của bạn)
bootfile=kernel.bin

# Lệnh thực thi (Macro uenvcmd tự động chạy khi boot)
# 1. In log thông báo
# 2. Load file từ thẻ nhớ (mmc 0:1) vào RAM
# 3. Dùng lệnh 'go' để nhảy tới địa chỉ đó (dành cho file .bin chưa có header)
uenvcmd=echo Booting RefARM-OS...; fatload mmc 0:1 ${loadaddr} ${bootfile}; go ${loadaddr}
```

* **Lưu ý:**
    - Cơ chế tự động load và thực thi `uEnv.txt` là hành vi của U-Boot do vendor cấu hình.
    - Khi sử dụng U-Boot custom trong tương lai, hành vi này có thể thay đổi.


### 3.4. kernel.bin
* **Vai trò:** Binary kernel của dự án OS reference.

* **Định dạng:** Raw Binary (được tạo từ kernel.elf bằng lệnh objcopy -O binary).

* **Cơ chế:** Được U-Boot load vào DDR tại địa chỉ 0x80000000 (khớp với Linker Script) và trao quyền điều khiển.