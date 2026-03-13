"""
Syscall Support for VinixOS Platform
Defines syscall numbers and generates syscall invocations
"""

from typing import Dict


# Syscall numbers (must match kernel/include/syscalls.h)
SYSCALL_NUMBERS: Dict[str, int] = {
    'write': 0,
    'exit': 1,
    'yield': 2,
    'read': 3,
}


def generate_syscall(syscall_name: str, args: list) -> list:
    """
    Generate ARM assembly for syscall invocation
    
    AAPCS syscall convention:
    - r7: syscall number
    - r0-r3: arguments (up to 4)
    - svc #0: invoke syscall
    - r0: return value
    
    Args:
        syscall_name: Name of syscall (write, read, exit, yield)
        args: List of argument registers
    
    Returns:
        List of assembly instructions
    """
    if syscall_name not in SYSCALL_NUMBERS:
        return [f"    @ Unknown syscall: {syscall_name}"]
    
    syscall_num = SYSCALL_NUMBERS[syscall_name]
    instructions = []
    
    # Load syscall number into r7
    instructions.append(f"    mov r7, #{syscall_num}")
    
    # Arguments already in r0-r3 (handled by caller)
    
    # Invoke syscall
    instructions.append(f"    svc #0")
    
    # Return value in r0
    
    return instructions


def is_syscall(function_name: str) -> bool:
    """
    Check if function is a syscall
    
    Args:
        function_name: Function name
    
    Returns:
        True if syscall, False otherwise
    """
    return function_name in SYSCALL_NUMBERS
