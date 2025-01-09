set print pretty
set debuginfod enabled on

break main
commands 1
    print argv
    continue
end

break free_lines
commands 2
    print *grep->query
end

run file.txt better
