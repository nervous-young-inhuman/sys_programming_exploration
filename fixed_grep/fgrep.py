#!/usr/bin/env python
def main(pat, file):
    n_pat_len = len(pat)
    lineno = 0
    for (line) in (open(file, 'r')):
        n_len = len(line)
        lineno += 1
        search_start = 0
        while search_start < n_len:
            try:
                got_index = line[search_start:].index(pat)
                print(f'{lineno}:{search_start + got_index} {pat}')
                search_start = search_start + got_index + n_pat_len
            except ValueError:
                break

def sliding_window(line, window_len):
    for i in range((len(line) - window_len) + 1):
        yield i, line[i:i+window_len]
            
def main(pat, file):
    pat_len = len(pat)
    with open(file, 'r') as fh:
        matches = (
            (lineno, colno)
            for (lineno, line) in enumerate(fh, 1)
            for (colno, chunk) in sliding_window(line, pat_len)
            if chunk == pat
        )
        for (lineno, colno) in matches:
            print(f'{lineno}:{colno} {pat}')

if __name__ == '__main__':
    import sys
    argv = sys.argv
    main(argv[1], argv[2])

