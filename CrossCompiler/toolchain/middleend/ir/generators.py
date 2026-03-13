"""
Temporary and Label Generators
Generate unique names for temporaries and labels
"""


class TempGenerator:
    """
    Generate unique temporary variable names
    Format: t0, t1, t2, ...
    """
    
    def __init__(self):
        """Initialize counter"""
        self.counter = 0
    
    def new_temp(self) -> str:
        """
        Generate new temporary variable name
        
        Returns:
            Temporary name (e.g., "t0", "t1")
        """
        temp = f"t{self.counter}"
        self.counter += 1
        return temp
    
    def reset(self) -> None:
        """Reset counter (for new function)"""
        self.counter = 0


class LabelGenerator:
    """
    Generate unique label names
    Format: L0, L1, L2, ...
    """
    
    def __init__(self):
        """Initialize counter"""
        self.counter = 0
    
    def new_label(self) -> str:
        """
        Generate new label name
        
        Returns:
            Label name (e.g., "L0", "L1")
        """
        label = f"L{self.counter}"
        self.counter += 1
        return label
    
    def reset(self) -> None:
        """Reset counter (for new function)"""
        self.counter = 0
