import random
import operator

class Number:
    def __init__(self):
        self.value = random.randrange(-99, 100)

    def __str__(self):
        return str(self.value)

class BinOp:
    REC_LEVEL = -1

    def __init__(self):
        BinOp.REC_LEVEL += 1
        if BinOp.REC_LEVEL < 10:
            choices = [Number, BinOp]
        else:
            choices = [Number]
        
        self.left = random.choice(choices)()
        self.right = random.choice(choices)()
        self.op = random.choice(["+", "-", "*", "/", "**"])
        self.has_brackets = random.choice([True, False])

    def __str__(self):
        l_paren = "(" if self.has_brackets else ""
        r_paren = ")" if self.has_brackets else ""
        return f"{l_paren}{self.left}{self.op}{self.right}{r_paren}"
    

for _ in range(100):
    try:
        expr = str(BinOp())
        result = eval(expr)
        expr = expr.replace("**", "^")
        if abs(result) < 1e9 and result % int(result) == 0.0:
            s = f"TEST_SOURCE_TO_INTERP(\"{expr}\", create_ast_integer({int(result)}));"
            print(s)
    except (ZeroDivisionError, OverflowError):
        ...
