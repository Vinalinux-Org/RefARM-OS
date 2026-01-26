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
* **Nội dung thực tế (cho Raw Binary Kernel):**

```bash
# Serial console configuration
console=ttyO0,115200n8

# DDR load address (must match linker script)
loadaddr=0x80000000

# Kernel binary filename (raw binary, NOT uImage!)
bootfile=kernel.bin

# Load macro (separated for easier debugging)
loadkernel=fatload mmc 0:1 ${loadaddr} ${bootfile}

# Auto-execute command (called by U-Boot)
# 1. Print boot message
# 2. Run loadkernel macro to load file from SD card
# 3. Use 'go' command to jump to raw binary entry point
uenvcmd=echo Booting RefARM-OS...; run loadkernel; go ${loadaddr}
```

* **Giải thích chi tiết:**
    - `console=ttyO0,115200n8`: Cấu hình serial console (UART0, baud 115200, 8 data bits, no parity, 1 stop bit)
    - `loadaddr=0x80000000`: Địa chỉ DDR để load kernel - **PHẢI KHỚP** với địa chỉ entry point trong linker script
    - `bootfile=kernel.bin`: Tên file kernel - **PHẢI LÀ .bin** (raw binary), KHÔNG phải uImage
    - `loadkernel`: Macro tách riêng để dễ debug - load file từ MMC device 0, partition 1
    - `uenvcmd`: Command tự động thực thi - dùng `run loadkernel` để call macro

* **Lưu ý quan trọng:**
    - **Dùng `go` command**, KHÔNG phải `bootm`
    - `go`: Nhảy đến địa chỉ raw binary (không parse header)
    - `bootm`: Dành cho uImage format (có 64-byte U-Boot header)
    - Nếu dùng nhầm `bootm` cho raw binary → U-Boot sẽ parse 64 byte đầu tiên làm header → entry point sai → crash
    - Địa chỉ 0x80000000 là DDR base trên AM335x, phải khớp tuyệt đối với linker script
    - Cơ chế tự động load và thực thi `uEnv.txt` là hành vi của U-Boot do vendor cấu hình
    - Khi sử dụng U-Boot custom trong tương lai, có thể cần điều chỉnh cách load này


### 3.4. kernel.bin
* **Vai trò:** Binary kernel của dự án OS reference.

* **Định dạng:** Raw Binary (được tạo từ kernel.elf bằng lệnh objcopy -O binary).

* **Cơ chế:** Được U-Boot load vào DDR tại địa chỉ 0x80000000 (khớp với Linker Script) và trao quyền điều khiển.

* **Build command:**
  ```bash
  arm-none-eabi-objcopy -O binary kernel.elf kernel.bin
  ```

* **Verification:**
  ```bash
  # Check file type
  file kernel.bin
  # Output: data (correct for raw binary)
  
  # Check size (should be a few KB for minimal kernel)
  ls -lh kernel.bin
  
  # Verify no VFP instructions (critical!)
  arm-none-eabi-objdump -d kernel.elf | grep -E "vld|vst|vmov"
  # Output must be EMPTY
  ```

---

## 4. Deployment Process

### 4.1. Prerequisites
- SD card ≥ 2GB, formatted with FAT32 partition
- Boot flag set on partition 1
- Files ready: MLO, u-boot.img, uEnv.txt, kernel.bin

### 4.2. Deployment Order (CRITICAL!)

**Thứ tự copy files là QUAN TRỌNG:**

```bash
# 1. Format SD card (if needed)
sudo mkfs.vfat -F 32 -n BOOT /dev/sdX1

# 2. Mount
sudo mount /dev/sdX1 /mnt

# 3. Copy MLO FIRST (must be at beginning of FAT)
sudo cp MLO /mnt/

# 4. Copy other files
sudo cp u-boot.img /mnt/
sudo cp uEnv.txt /mnt/
sudo cp kernel.bin /mnt/

# 5. Verify
ls -lh /mnt/
cat /mnt/uEnv.txt

# 6. Sync and unmount
sync
sudo umount /mnt
```

**Tại sao MLO phải copy đầu tiên:**
- Boot ROM của AM335x chỉ có FAT parser đơn giản
- MLO phải nằm ở cluster đầu tiên của filesystem
- Nếu copy files khác trước, MLO có thể bị fragment → Boot ROM không load được

---

## 5. Troubleshooting Common Boot Issues

### 5.1. "Unable to read file kernel.bin"

**Triệu chứng UART log:**
```
reading kernel.bin
** Unable to read file kernel.bin **
## Starting application at 0x80000000 ...
undefined instruction
pc : [<80000008>]
```

**Nguyên nhân:**
- File không tồn tại trên SD card
- Filename sai (case-sensitive: `kernel.bin` không phải `Kernel.bin`)
- SD card partition corrupt hoặc không mount được

**Giải pháp:**
```bash
# Verify files on SD card
sudo mount /dev/sdX1 /mnt
ls -lh /mnt/ | grep kernel
# Must show: kernel.bin (not kernel.bin.bin or other name!)

# Check file size
ls -l /mnt/kernel.bin
# Must be > 0 bytes

# Re-deploy if needed
sudo cp kernel.bin /mnt/
sync
sudo umount /mnt
```

---

### 5.2. "undefined instruction" immediately after "Starting application"

**Triệu chứng:**
```
## Starting application at 0x80000000 ...
undefined instruction
Resetting CPU ...
```

**Nguyên nhân:**
1. U-Boot load failed → executing garbage memory
2. Kernel compiled with VFP/NEON instructions (VFP chưa enable trong boot)
3. Wrong compiler flags (hard-float ABI)
4. Kernel entry point address mismatch

**Giải pháp:**

**Check 1: Verify kernel loaded successfully**
```
# In U-Boot log, look for:
reading kernel.bin
XXX bytes read in Y ms   # ← This line MUST appear with non-zero bytes
```

**Check 2: Verify no VFP instructions**
```bash
arm-none-eabi-objdump -d build/kernel.elf | grep -E "vld|vst|vmov|vadd|vsub"
# Output MUST be empty!
```

**Check 3: Verify compiler flags**
```bash
# In Makefile, must have:
CFLAGS = ... -mfloat-abi=soft ...   # NOT hard!
CROSS_COMPILE = arm-none-eabi-      # NOT arm-linux-gnueabihf-
```

**Check 4: Verify entry point**
```bash
arm-none-eabi-readelf -h build/kernel.elf | grep Entry
# Entry point address: 0x80000000  # ← Must be this address

# Verify linker script
grep ORIGIN linker/kernel.ld
# ORIGIN = 0x80000000  # ← Must match
```

---

### 5.3. No UART output at all

**Triệu chứng:**
- No U-Boot banner
- No kernel output
- Blank serial console

**Nguyên nhân:**
1. UART wiring incorrect
2. Serial console settings wrong
3. Board not booting (hardware issue)
4. Boot device priority wrong

**Giải pháp:**

**Check 1: UART wiring (J1 header on BBB)**
```
BBB J1 Pin    USB-Serial Adapter
Pin 1 (GND) → GND
Pin 4 (RX)  → TX    # ← Cross-over!
Pin 5 (TX)  → RX    # ← Cross-over!
```

**Check 2: Serial settings**
```bash
# Terminal settings:
# - Baud rate: 115200
# - Data bits: 8
# - Parity: None
# - Stop bits: 1
# - Flow control: None

# Test connection
screen /dev/ttyUSB0 115200
# or
minicom -D /dev/ttyUSB0 -b 115200
```

**Check 3: Boot device priority**
- Hold BOOT button (S2) while powering on to force SD card boot
- Check boot pins configuration

---

### 5.4. Kernel boots but immediately resets (watchdog)

**Triệu chứng:**
```
RefARM-OS
Boot OK
[... after 60 seconds ...]
Resetting CPU ...
```

**Nguyên nhân:**
- Watchdog not disabled in kernel
- WDT1 still running from bootloader

**Giải pháp:**
- Verify `disable_watchdog()` is called in kernel_main()
- Check watchdog registers write sequence
- Ensure clock for WDT1 is enabled before accessing registers

---

### 5.5. U-Boot loads wrong file or old kernel

**Triệu chứng:**
- Old kernel output appears
- Modifications not reflected
- Unexpected behavior

**Nguyên nhân:**
- File cache not synced
- Wrong partition mounted
- Multiple kernel files with different names

**Giải pháp:**
```bash
# Always sync after copy
sudo cp kernel.bin /mnt/
sync  # ← CRITICAL! Wait for write to complete
sudo umount /mnt

# Remove old files
sudo mount /dev/sdX1 /mnt
sudo rm /mnt/kernel.bin.old
sudo rm /mnt/uImage  # Remove if exists
sync
sudo umount /mnt

# Verify timestamp
sudo mount /dev/sdX1 /mnt
ls -lh /mnt/kernel.bin
# Check modification time matches your build
sudo umount /mnt
```

---

## 6. Expected Boot Success Indicators

**Full successful boot log should show:**

```
U-Boot SPL 2017.05-rc2 (May 02 2017 - 08:53:40)
Trying to boot from MMC1

U-Boot 2017.05-rc2 (May 02 2017 - 08:53:40 +0530)
[... U-Boot initialization ...]

reading uEnv.txt
179 bytes read in 3 ms (57.6 KiB/s)          # ← uEnv.txt loaded
Loaded env from uEnv.txt
Importing environment from mmc0 ...
Running uenvcmd ...
Booting RefARM-OS...                          # ← uenvcmd executing
reading kernel.bin
425 bytes read in 3 ms (138.7 KiB/s)          # ← kernel.bin loaded (size > 0!)
## Starting application at 0x80000000 ...

RefARM-OS                                     # ← Kernel output!
Boot OK                                       # ← Success!
```

**Key success indicators:**
- ✅ uEnv.txt loaded (XXX bytes read)
- ✅ kernel.bin loaded (XXX bytes read, non-zero)
- ✅ "Starting application at 0x80000000" (correct address)
- ✅ Kernel output appears ("RefARM-OS", "Boot OK")
- ✅ No reset loop or crash
- ✅ System stable in halt state

---