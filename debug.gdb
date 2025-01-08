set print pretty
set debuginfod enabled on

break main
commands 1
    print argv
    continue
end

break free_file
commands 2
    print *grep->lines@grep->lines_count
end

run file.txt in
