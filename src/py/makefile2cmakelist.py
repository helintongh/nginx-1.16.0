#!/usr/bin/python
import os
import sys
import re

total_line = 0

def get_line_by_kw(file, ptn, key_word):
	r_cmd = os.popen(" ".join(["grep -n", key_word, file]))
	ret_s = r_cmd.readline()
	r_cmd.close()
	line_ptn = re.compile(ptn)
	line_s = line_ptn.search(ret_s).group()[:-1]
	return line_s

def write_a_line(write_l, content_str, line_start, line_end, tofile, postfix):
	global total_line
	if (write_l <= total_line):
		line = "sed -in \'%dc %s\' %s" %(write_l, content_str[line_start:line_end] + postfix, tofile)
	else:
		line = "sed -in \'%da %s\' %s" %(write_l - 1, content_str[line_start:line_end] + postfix, tofile)
	os.system(line)
	line = "sed -in \'%ds/^/        /\' %s" %(write_l, tofile)
	os.system(line)

def insert_a_line(insert_l, content_str, tofile):
	line = "sed -in \"%da %s\" %s" %(insert_l, content_str, tofile)
	os.system(line)

def write_header_to_cmake(start_line, content_list, tofile, toline):
	write_l = toline
	read_l = start_line

	write_a_line(write_l, content_list[read_l], len("XXXX_DEPS = "), -3, tofile, "")

	while True:
		if content_list[read_l][-2] == '\\':
			read_l += 1
			write_l += 1
			write_a_line(write_l, content_list[read_l], 1, -3, tofile, "")
		else:
			write_a_line(write_l, content_list[read_l], 1, -1, tofile, "")
			break
	return write_l

def write_dotc_set_clfags(start_line, content_list, fromfile, tofile, toline, set_n, cflag_n):
	write_l = toline
	read_l = start_line

	all_flags_l = list()

	while content_list[read_l][-2] == '\\':
		read_l += 1
		if '.o' in content_list[read_l]:
			if '.o' in content_list[read_l + 1]:
				if content_list[read_l].startswith("	objs/src"):
					write_a_line(write_l, content_list[read_l], len("	objs/"), -4, tofile, "c")
				else:
					if content_list[read_l].startswith("	objs/addon"):
						addon_ln = int(get_line_by_kw(fromfile, r"^\d+:", content_list[read_l][1:-3] + ':'))
						write_a_line(write_l, content_list[addon_ln], 1, -1, tofile, "")
					else:
						write_a_line(write_l, content_list[read_l], 1, -4, tofile, "c")
			else:
				if content_list[read_l].startswith("	objs/src") or content_list[read_l].startswith("	objs/addon"):
					write_a_line(write_l, content_list[read_l], len("	objs/"), -4, tofile, "c)")
				else:
					if content_list[read_l].startswith("	objs/addon"):
						addon_ln = int(get_line_by_kw(fromfile, r"^\d+:", content_list[read_l][1:-3] + ':'))
						write_a_line(write_l, content_list[addon_ln], 1, -1, tofile, "")
					else:
						write_a_line(write_l, content_list[read_l], 1, -4, tofile, "c)")
			write_l += 1
		else:
			if '-' in content_list[read_l]:
				if content_list[read_l].startswith('	'):
					all_flags_l.append(content_list[read_l][1:-1].replace('\\', ''))
				else:
					all_flags_l.append(content_list[read_l][:-1].replace('\\', ''))

	all_flags_l.append(content_list[cflag_n][len("CFLAGS = "): -1])
	print (" ".join(all_flags_l))
	set_line = r"set\(CMAKE_C_FLAGS \"\$\{CMAKE_C_FLAGS\} -L /usr/lib64/ %s\"\)" %(" ".join(all_flags_l))
	insert_a_line(set_n, set_line, tofile)
	line = "sed -in \"/add_executable/{x;p;x}\" %s" %(tofile)
	os.system(line)

	return write_l + 2

def set_include_directories(makefile, cmakelist):
	added_list = list()

	r_cmd = os.popen(' '.join(["grep -n include_directories", cmakelist]))
	inc_dir_l = r_cmd.readlines()
	r_cmd.close()

	line_ptn = re.compile(r"^\d+:")
	start_line_ns = line_ptn.search(inc_dir_l[0]).group()[:-1]
	start_line_n = int(start_line_ns)

	r_cmd = os.popen(' '.join(["grep -n \"I \"", makefile]))
	inc_path_l = r_cmd.readlines()
	r_cmd.close()
	inc_pth_ptn = re.compile(r"-I\s*\S+\b")

	insert_a_line(start_line_n, r"include_directories\(/usr/local/include\)", cmakelist)

	for item in inc_path_l:
		flag = 0
		pth_s = inc_pth_ptn.search(item).group()[3:].replace('\\', '')
		for inc_dir_item in inc_dir_l:
			if pth_s in inc_dir_item or pth_s in added_list:
				flag = 1
				break
		if flag == 0:
			added_list.append(pth_s)
			set_line = r"include_directories\(%s\)" %pth_s
			insert_a_line(start_line_n, set_line, cmakelist)

if __name__ == '__main__':
	if len(sys.argv) != 3:
		print "Error! Usage is ./makefile2cmakelist.py $(path)/Makefile $(path)/CMakeList"
		sys.exit()

	if False == os.path.isfile(sys.argv[1]) or False == os.path.isfile(sys.argv[2]):
		print "Error! Parameter should be a file!"
		sys.exit()
	else:
		if "Makefile" in sys.argv[1]:
			fm = 1
			if "CMakeList" in sys.argv[2]:
				fc = 2
			else:
				print("Error! There is no file named \"CMakeList\" in paramters!")
				sys.exit()
		else:
			if "Makefile" in sys.argv[2]:
				fm = 2
				if "CMakeList" in sys.argv[1]:
					fc = 1
				else:
					print("Error! There is no file named \"CMakeList\" in paramters!")
					sys.exit()
			else:
				print("Error! There is no file named \"Makefile\" in paramters!")
				sys.exit()

	if False == os.path.isfile(sys.argv[fc] + '.bak'):
		line = "cp %s %s" %(sys.argv[fc], sys.argv[fc] + '.bak')
	else:
		line = "rm -rf %s; cp -rf %s %s" %(sys.argv[fc], sys.argv[fc] + '.bak', sys.argv[fc])
	os.system(line)

	line = "wc -l %s" %sys.argv[fc]
	r_cmd = os.popen(line)
	ret_s = r_cmd.readline()
	print "wc resault:"+ret_s[0:3]
	r_cmd.close()
	t_line_ptn = re.compile("^\d+\s")
	global total_line
	total_line = int(t_line_ptn.search(ret_s).group()[:-1])

	key_ns = get_line_by_kw(sys.argv[fm], r"^\d+:", "CORE_DEPS")
	print ("CORE_DEPS " + key_ns)
	core_deps_n = int(key_ns) - 1

	key_ns = get_line_by_kw(sys.argv[fm], r"^\d+:", "HTTP_DEPS")
	print ("HTTP_DEPS " + key_ns)
	http_deps_n = int(key_ns) - 1

	key_ns = get_line_by_kw(sys.argv[fm], r"^\d+:", "\(LINK\)")
	print ("LINK " + key_ns)
	dotc_n = int(key_ns) - 1

	key_ns = get_line_by_kw(sys.argv[fm], r"^\d+:", "\"CFLAGS =\"")
	print ("CFLAGS " + key_ns)
	cflag_n = int(key_ns) - 1

	key_ns = get_line_by_kw(sys.argv[fc], r"^\d+:", "add_executable")
	print ("add_executable " + key_ns)
	exe_n = int(key_ns) + 1

	mf = open(sys.argv[fm], "r+")

	df = mf.readlines()
	print(df[core_deps_n])
	print(df[http_deps_n])
	print(df[dotc_n])
	print(df[cflag_n])

	mf.close()

	old_exe_n = exe_n
	set_n = exe_n - 2
	exe_n = write_header_to_cmake(core_deps_n, df, sys.argv[fc], old_exe_n)
	old_exe_n = exe_n + 1
	exe_n = write_header_to_cmake(http_deps_n, df, sys.argv[fc], old_exe_n)
	old_exe_n = exe_n + 1
	exe_n = write_dotc_set_clfags(dotc_n, df, sys.argv[fm], sys.argv[fc], old_exe_n, set_n, cflag_n)

	line = "sed -in \'%d,$d\' %s" %(exe_n, sys.argv[fc])
	os.system(line)

	set_include_directories(sys.argv[fm], sys.argv[fc])

