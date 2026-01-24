# Memory Map (AM335x / BeagleBone Black)

## 1. Mục đích tài liệu

Tài liệu này liệt kê các vùng nhớ và địa chỉ phần cứng của SoC AM335x được sử dụng trong dự án OS reference.

Nội dung trong file này:
- Chỉ phản ánh thông tin từ datasheet / TRM.
- Không chứa quyết định thiết kế hệ điều hành.
- Không chỉ định vùng nhớ nào được dùng cho kernel, stack hay heap.

Mọi quyết định sử dụng các vùng nhớ này sẽ được thực hiện ở các tài liệu và file khác (linker script, boot flow, driver).

---

## 2. Tổng quan không gian địa chỉ

AM335x sử dụng không gian địa chỉ 32-bit.

Các vùng chính bao gồm:
- On-chip Boot ROM (Internal)
- On-chip SRAM (Internal)
- External DDR memory
- Peripheral memory-mapped I/O

---

## 3. External DDR Memory

External DDR là vùng nhớ chính dùng cho hệ điều hành và dữ liệu runtime.

- Base address: `0x80000000`
- Size: phụ thuộc cấu hình board (thường 512 MB trên BeagleBone Black)

```text
      High Address
           |
+----------------------------+ 0x9FFFFFFF (End of RAM)
|                            |
|                            |
|          DDR RAM           |
|         (512 MB)           |
|                            |
|                            |
+----------------------------+ 0x80000000 (Base of RAM)
           |
      Low Address
```

---

## 4. On-chip SRAM (OCMC RAM)

AM335x cung cấp vùng SRAM nội dùng cho boot (SPL) hoặc firmware nhỏ.

- Base address: `0x402F0400`
- Size: 64 KB

```text
      High Address
           |
+----------------------------+ 0x402FFFFF (End of SRAM)
|                            |
|                            |
|          OCMC RAM          |
|      (Internal SRAM)       |
|      (Size: ~64 KB)        |
|                            |
|                            |
+----------------------------+ 0x402F0400 (Base of SRAM)
           |
      Low Address
```

Lưu ý: Boot ROM sẽ load SPL (MLO) vào địa chỉ đầu của vùng này.

---

## 5. On-chip ROM (Boot ROM)

On-chip ROM chứa boot ROM của SoC.

- Base address: `0x40000000`
- Size: 128 KB (thường là 0x40000000 - 0x4001FFFF)

```text
High Address
           |
+----------------------------+ 0x4001FFFF
|                            |
|      On-chip Boot ROM      |
|      (Read Only / Code)    |
|                            |
+----------------------------+ 0x40000000 (Base)
           |
      Low Address
```

Lưu ý:
- Không thể ghi.
- Không sử dụng trực tiếp cho kernel.

---

## 6. Peripheral Memory Map

Các peripheral được truy cập thông qua memory-mapped I/O.
Danh sách dưới đây chỉ bao gồm các peripheral tối thiểu cần thiết để Boot và Debug OS.

### 6.1. System & Control (Bắt buộc)

Để hệ thống chạy được, cần cấu hình Clock và tắt Watchdog.
- **Clock Module (CM)**
    - Base: `0x44E00000`
    - Dùng để bật clock cho UART, Timer, GPIO.
- **Watchdog Timer (WDT1)**
    - Base: `0x44E35000`
    - **Quan trọng**: Cần disable WDT1 trong vòng 1-2 phút đầu tiên, nếu không board sẽ tự reset.

### 6.2. UART (Console)

- UART0 base address: `0x44E09000`

```text
      High Address
           |
+----------------------------+ 0x44E09FFF (Base + 4KB)
|                            |
|           UART0            |
|      (Debug Console)       |
|                            |
+----------------------------+ 0x44E09000 (Base)
           |
      Low Address
```

---

### 6.3. Timer (Scheduler)

- DMTimer2 base address: `0x48040000`

```text
      High Address
           |
+----------------------------+ 0x48040FFF (Base + 4KB)
|                            |
|          DMTimer2          |
|      (System Timer)        |
|                            |
+----------------------------+ 0x48040000 (Base)
           |
      Low Address
```
---

### 6.4. Interrupt Controller

- Interrupt Controller base address: `0x48200000`

```text
      High Address
           |
+----------------------------+ 0x48200FFF (Base + 4KB)
|                            |
|            INTC            |
|    (Interrupt Controller)  |
|                            |
+----------------------------+ 0x48200000 (Base)
           |
      Low Address
```

---

## 7. Ghi chú quan trọng

- Tất cả địa chỉ trong tài liệu này là địa chỉ vật lý.
- MMU được giả định là **TẮT** (Disable) khi boot bắt đầu.
- Không có giả định về mapping ảo trong giai đoạn boot.
- Khi truy cập Peripheral, cần đảm bảo Clock cho module đó đã được bật (tại địa chỉ `0x44E0xxxx`).
- Code không được hard-code địa chỉ, hãy sử dụng file header định nghĩa (ví dụ `mem_map.h`).

---

## 8. Phạm vi tài liệu

Tài liệu này:
- Không mô tả cách sử dụng các vùng nhớ.
- Không mô tả layout kernel.
- Không mô tả stack hoặc heap.

Các nội dung trên được mô tả ở:
- `docs/02_boot/linker_layout.md`
- `linker/kernel.ld`
- Các tài liệu kernel và driver tương ứng.
