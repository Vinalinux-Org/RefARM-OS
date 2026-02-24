# Reset Sequence

Tài liệu này mô tả trình tự khởi động của kernel từ entry point `_start`
cho đến khi C code bắt đầu thực thi tại `kernel_main()`.

Mục tiêu của reset sequence là:
- Xác định rõ các bước tối thiểu để đưa CPU vào trạng thái xác định
- Thiết lập môi trường runtime tối thiểu để gọi C code an toàn
- Giữ đúng phạm vi “boot tối thiểu” của dự án (BOOT STABLE)

Tài liệu này bám sát sơ đồ `diagrams/reset_to_c.mmd` và mã nguồn:
- `src/arch/arm/boot/reset.S`
- `src/arch/arm/boot/entry.S`
- `src/kernel/main.c`
- `linker/kernel.ld`

---

## 1. Phạm vi và nguyên tắc

1.1. Phạm vi bắt đầu tại thời điểm kernel nhận quyền điều khiển ở `_start`.

1.2. Các giai đoạn trước `_start` (Boot ROM, SPL/MLO, U-Boot hoặc bootloader khác)
nằm ngoài phạm vi triển khai của kernel. Kernel chỉ dựa vào “hợp đồng boot”
đã được nêu trong `boot_contract.md`.

1.3. Boot tối thiểu trong tài liệu này không bao gồm:
- Bật interrupt (IRQ/FIQ)
- Timer / scheduler
- MMU / virtual memory
- Driver đầy đủ ngoài UART debug (nếu có)

1.4. Mục tiêu của boot tối thiểu:
- Thấy log UART ổn định trên board thật
- Dòng thực thi `_start → reset.S → entry.S → kernel_main()` là deterministic

---

## 2. Trạng thái đầu vào tại `_start`

2.1. Trạng thái phần cứng tại `_start` được giả định theo `boot_contract.md`:
- DDR đã sẵn sàng
- Kernel image đã được load vào DDR tại địa chỉ đã link
- MMU OFF
- CPU ở chế độ privileged

2.2. Kernel không giả định stack từ bootloader là hợp lệ hoặc ổn định.
Do đó kernel tự thiết lập stack ngay trong giai đoạn boot.

---

## 3. Bước 1: `_start` (Kernel Entry)

3.1. `_start` là entry point chính thức của kernel (được chỉ định bởi linker script).

3.2. `_start` không phải nơi triển khai logic hệ điều hành.
Vai trò của `_start` là điểm bắt đầu của chuỗi boot kernel và chuyển giao
sang các bước “CPU hygiene” trong `reset.S`.

3.3. Nguyên tắc thực thi:
- Không sử dụng stack trước khi stack được thiết lập rõ ràng
- Không gọi C code trước khi `.bss` được clear

---

## 4. Bước 2: `reset.S` (CPU Hygiene)

`reset.S` chịu trách nhiệm đưa CPU về trạng thái xác định và tối thiểu để
kernel có thể tiếp tục thiết lập môi trường runtime.

### 4.1. Mask IRQ/FIQ

4.1.1. IRQ và FIQ phải được mask ở đầu boot để tránh:
- Nhảy vào handler khi vector table chưa sẵn sàng
- Phụ thuộc vào trạng thái interrupt từ bootloader

4.1.2. Trong giai đoạn boot tối thiểu, IRQ/FIQ được giữ ở trạng thái mask
cho đến khi kernel có mô hình interrupt rõ ràng.

### 4.2. Thiết lập CPU mode về SVC

4.2.1. Kernel chuẩn hóa CPU mode về SVC (Supervisor mode) để:
- Có quyền privileged
- Dễ kiểm soát stack và flow của boot

4.2.2. Kernel không dựa vào việc bootloader đã set mode chính xác.

### 4.3. Thiết lập vector base (VBAR)

4.3.1. Vector table của kernel nằm trong kernel image (trong DDR),
       được định nghĩa trong section `.vectors` của reset.S.

4.3.2. Reset handler thiết lập thanh ghi VBAR (Vector Base Address Register)
       trỏ đến địa chỉ `_vector_start` (symbol từ linker).

4.3.3. **Instruction Synchronization Barrier (ISB):**
       
       Sau khi write vào VBAR (CP15 coprocessor register), phải thực thi
       ISB instruction để đảm bảo write operation hoàn tất.

       Code sequence trong reset.S:
       ```asm
       ldr r0, =_vector_start
       mcr p15, 0, r0, c12, c0, 0   # Write VBAR
       isb                          # Instruction barrier  
       b   entry
       ```

       ISB là best practice cho tất cả CP15 coprocessor writes.

4.3.4. Trong giai đoạn boot tối thiểu:
- Vector table chủ yếu là stub handlers (infinite loops)
- IRQ/FIQ vẫn masked, nên vectors chưa được invoke thực tế
- Việc setup VBAR sớm phục vụ tính "đúng kiến trúc"

---

## 5. Bước 3: `entry.S` (Prepare C Environment)

`entry.S` chịu trách nhiệm chuẩn bị môi trường runtime tối thiểu để gọi C code
một cách an toàn và deterministic.

### 5.1. Setup stacks

5.1.1. Kernel thiết lập stack riêng cho các CPU mode cần thiết.

5.1.2. Trong giai đoạn boot tối thiểu:
- Stack SVC là bắt buộc
- Stack IRQ được thiết lập sớm để sẵn sàng cho giai đoạn enable interrupt sau này

5.1.3. Kích thước và vị trí stack được định nghĩa bởi linker script (`kernel.ld`)
và được export qua các symbol:
- `_svc_stack_top`
- `_irq_stack_top`

5.1.4. Nguyên tắc:
- Stack grows downward
- Không dùng stack của bootloader

### 5.2. Clear `.bss`

5.2.1. `.bss` chứa các biến global/static chưa khởi tạo.
Nếu không được clear, giá trị sẽ là rác và gây boot không ổn định.

5.2.2. `entry.S` zero toàn bộ vùng `.bss` dựa trên symbol linker:
- `_bss_start`
- `_bss_end`

5.2.3. Đây là điều kiện bắt buộc trước khi gọi bất kỳ C code nào.

### 5.3. Gọi `kernel_main()`

5.3.1. Sau khi stack hợp lệ và `.bss` đã được clear, `entry.S` chuyển quyền
điều khiển sang `kernel_main()` bằng lệnh `BL`.

5.3.2. `kernel_main()` là điểm bắt đầu chính thức của kernel C code.
Từ thời điểm này, boot stage kết thúc.

---

## 6. Bước 4: `kernel_main()` (C Entry)

6.1. `kernel_main()` là entry point của C code trong kernel.

6.2. Trong checkpoint BOOT STABLE hiện tại, `kernel_main()` chỉ thực hiện:
- Log UART tối thiểu để xác nhận boot thành công
- Không enable interrupt
- Không init subsystem phức tạp

6.3. Nếu `kernel_main()` trả về, kernel sẽ rơi vào dead loop (`b .`)
để đảm bảo hệ thống không chạy vào vùng không xác định.

---

## 7. Liên kết với linker layout

7.1. Linker script (`kernel.ld`) là nguồn sự thật duy nhất về:
- vị trí vector table
- vị trí `.bss`
- vị trí stack

7.2. Reset sequence phụ thuộc trực tiếp vào các symbol sau:
- `_vector_start` (hoặc symbol tương ứng cho vector base)
- `_bss_start`, `_bss_end`
- `_svc_stack_top`, `_irq_stack_top`

7.3. Khi có thay đổi linker layout, tài liệu này phải được cập nhật để khớp
với các symbol và phạm vi được sử dụng trong `reset.S` và `entry.S`.

---

## 8. Tiêu chí BOOT STABLE cho reset sequence

8.1. Các điều kiện tối thiểu để coi reset sequence ổn định:
- Kernel luôn in ra log UART sau mỗi lần boot/reset
- Không phụ thuộc hành vi ngẫu nhiên hoặc trạng thái còn sót từ bootloader
- Dòng thực thi đúng thứ tự `_start → reset.S → entry.S → kernel_main()`

8.2. Các cải tiến sau BOOT STABLE sẽ được thực hiện ở giai đoạn kế tiếp:
- Tách riêng `vector_table.S` (nếu cần)
- Bổ sung exception handlers
- Enable interrupt và timer

---