import tokenize
import io

def print_tokens(s):
    for tok in tokenize.tokenize(io.BytesIO(s.encode('utf-8')).readline):
        print(tok)

print_tokens("[1, 2, 3]")
