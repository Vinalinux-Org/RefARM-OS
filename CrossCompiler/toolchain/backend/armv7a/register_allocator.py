"""
Register Allocator for ARMv7-A
Manages register allocation and spilling
"""

from typing import Dict, Set, Optional


class RegisterAllocator:
    """
    Simple register allocator for ARMv7-A
    
    Available registers: r4-r11 (8 registers)
    Reserved registers:
    - r0-r3: argument passing and return value
    - r12 (ip): intra-procedure scratch
    - r13 (sp): stack pointer
    - r14 (lr): link register
    - r15 (pc): program counter
    """
    
    AVAILABLE_REGISTERS = ['r4', 'r5', 'r6', 'r7', 'r8', 'r9', 'r10', 'r11']
    
    def __init__(self):
        """Initialize register allocator"""
        self.free_registers: Set[str] = set(self.AVAILABLE_REGISTERS)
        self.allocated: Dict[str, str] = {}  # variable/temp name → register
        self.spilled: Dict[str, int] = {}  # variable/temp name → stack offset
        self.used_registers: Set[str] = set()  # Track all used registers
        self.stack_offset = 0  # Current stack offset for spilled variables
    
    def allocate(self, name: str) -> str:
        """
        Allocate register for variable/temporary
        
        Args:
            name: Variable or temporary name
        
        Returns:
            Register name (e.g., "r4")
        """
        # Already allocated?
        if name in self.allocated:
            return self.allocated[name]
        
        # Try to allocate free register
        if self.free_registers:
            reg = self.free_registers.pop()
            self.allocated[name] = reg
            self.used_registers.add(reg)
            return reg
        
        # No free registers - need to spill
        # For simplicity, spill to stack and use r4 as scratch
        self.stack_offset += 4
        self.spilled[name] = self.stack_offset
        
        # Use r4 as scratch register for spilled variables
        return 'r4'
    
    def free(self, name: str) -> None:
        """
        Free register allocated to variable/temporary
        
        Args:
            name: Variable or temporary name
        """
        if name in self.allocated:
            reg = self.allocated[name]
            del self.allocated[name]
            self.free_registers.add(reg)
    
    def get_register(self, name: str) -> Optional[str]:
        """
        Get register allocated to variable/temporary
        
        Args:
            name: Variable or temporary name
        
        Returns:
            Register name if allocated, None otherwise
        """
        return self.allocated.get(name)
    
    def is_spilled(self, name: str) -> bool:
        """
        Check if variable/temporary is spilled to stack
        
        Args:
            name: Variable or temporary name
        
        Returns:
            True if spilled, False otherwise
        """
        return name in self.spilled
    
    def get_spill_offset(self, name: str) -> Optional[int]:
        """
        Get stack offset for spilled variable
        
        Args:
            name: Variable or temporary name
        
        Returns:
            Stack offset if spilled, None otherwise
        """
        return self.spilled.get(name)
    
    def get_used_registers(self) -> Set[str]:
        """
        Get set of all used registers (for prologue/epilogue)
        
        Returns:
            Set of register names
        """
        return self.used_registers
    
    def get_stack_size(self) -> int:
        """
        Get total stack size needed for spilled variables
        
        Returns:
            Stack size in bytes
        """
        return self.stack_offset
    
    def reset(self) -> None:
        """Reset allocator for new function"""
        self.free_registers = set(self.AVAILABLE_REGISTERS)
        self.allocated = {}
        self.spilled = {}
        self.used_registers = set()
        self.stack_offset = 0
