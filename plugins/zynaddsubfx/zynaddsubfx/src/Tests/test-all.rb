#!/usr/bin/ruby

files = Dir.glob("/usr/local/share/zynaddsubfx/banks/*/*.xiz")
f = open("out.csv", "w+")
files.each do |input|
    res = `ins-test "#{input}"`
    res = "\"#{input}\", #{res}"
    puts res
    f.puts res
end
f.close

