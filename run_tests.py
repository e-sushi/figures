#run_tests.py
#delle preset: python3 run_tests.py --p -t 2 -f *
#_______________________
#Command Line Arguments:
#-e [str]
#  executable path
#  default: root_dir + "build\\debug\\suugu.exe"
#
#-f [str]
#  test filter
#  default: "*"
#
#-t [int]
#  timeout (in seconds)
#  default: 3
#
#--p
#  print suugu output on error (above the failed/passed message)
#_______________________
#TODOs:

import sys,os,subprocess,ctypes,random

SolverError_None                           = 0
SolverError_NotImplemented                 = 1
SolverError_InvalidExpression              = 2
SolverError_DivideByZero                   = 3
SolverError_WrongNumberOfFunctionArguments = 4
SolverError_LogarithmBaseZero              = 5
SolverError_LogarithmBaseOne               = 6
SolverError_LogarithmOfZero                = 7

i1 = random.randint(0,100000);
i2 = random.randint(0,100000);
i3 = random.randint(0,100000);
f1 = random.random() * 100000;
f2 = random.random() * 100000;
f3 = random.random() * 100000;

#["group/validity/name", f"expression", f"expected_result"]
tests = [
	["single_variable/valid/solve",    f"x = {i1} + {i2}", f"x = {i1+i2}"],
	["single_variable/valid/solve",    f"{i1} + {i2} = x", f"x = {i1+i2}"],
	["single_variable/valid/solve",    f"x = {f1} + {f2}", f"x = {f1+f2}"],
	["single_variable/valid/solve",    f"{f1} + {f2} = x", f"x = {f1+f2}"],
	["single_variable/valid/addition", f"x + {i1} = {i2}", f"x = {i2-i1}"],
	["single_variable/valid/addition", f"{i1} + x = {i2}", f"x = {i2-i1}"],
	["single_variable/valid/addition", f"{i1} = {i2} + x", f"x = {i1-i2}"],
	["single_variable/valid/addition", f"{i1} = x + {i2}", f"x = {i1-i2}"],
	["single_variable/valid/addition", f"x + {f1} = {f2}", f"x = {f2-f1}"],
	["single_variable/valid/addition", f"{f1} + x = {f2}", f"x = {f2-f1}"],
	["single_variable/valid/addition", f"{f1} = {f2} + x", f"x = {f1-f2}"],
	["single_variable/valid/addition", f"{f1} = x + {f2}", f"x = {f1-f2}"],
];

def main():
	#gather command line args
	root_dir = os.path.dirname(__file__)
	suugu_exe_path = os.path.join(root_dir, "build\\debug\\suugu.exe")
	test_filter = "*"
	print_errors = False
	test_timeout = 3
	
	arg_index = 1;
	for _ in range(len(sys.argv)):
		if(_ != arg_index): continue;
		if(sys.argv[arg_index] == "-e"):
			if(arg_index+1 >= len(sys.argv)):
				print("ERROR: no argument passed to -e");
				return;
			elif(sys.argv[arg_index+1].startswith("-")):
				print("ERROR: invalid argument for -e:", sys.argv[arg_index+1]);
				return;
			else:
				suugu_exe_path = sys.argv[arg_index+1];
				arg_index += 1;
		elif(sys.argv[arg_index] == "-f"):
			if(arg_index+1 >= len(sys.argv)):
				print("ERROR: no argument passed to -f");
				return;
			elif(sys.argv[arg_index+1].startswith("-")):
				print("ERROR: invalid argument for -f:", sys.argv[arg_index+1]);
				return;
			else:
				test_filter = sys.argv[arg_index+1];
				arg_index += 1;
		elif(sys.argv[arg_index] == "-t"):
			if(arg_index+1 >= len(sys.argv)):
				print("ERROR: no argument passed to -t");
				return;
			elif(sys.argv[arg_index+1].startswith("-")):
				print("ERROR: invalid argument for -t:", sys.argv[arg_index+1]);
				return;
			else:
				test_timeout = int(sys.argv[arg_index+1]);
				arg_index += 1;
		elif(sys.argv[arg_index] == "--p"):
			print_errors = True
		else:
			print("ERROR: unknown flag: ", sys.argv[arg_index]);
			return;
		arg_index += 1;
		
	filter_wildcard = test_filter.endswith("*")
	
	#iterate thru tests
	tests_total  = 0;
	tests_passed = 0;
	tests_failed = 0;
	for path,expression,expected in tests:
		#print(path);
		#print(path.startswith(test_filter[0:-1]))
		
		if(filter_wildcard):
			if not(path.startswith(test_filter[0:-1])):
				#print(1);
				continue;
		elif(path != test_filter):
				#print(2);
				continue;
	
		tests_total += 1;
		path_arr = path.split('/');
		group = path_arr[-3];
		type  = path_arr[-2];
		name  = path_arr[-1];
		
		if(type == 'valid'): #valid tests should return errorlevel 0 from suugu.exe
			try:
				subprocess.run(compile_cmd, capture_output=True, check=True, encoding="utf-8", timeout=test_timeout);
				
				try:
					subprocess.run(file_exe, capture_output=True, check=True, encoding="utf-8", timeout=test_timeout);
					#print("%-60s %s (E: %d; A: %d)" % (path, "PASSED", expected, 0));
					print("%-60s %s" % (path, "PASSED"));
					tests_passed += 1;
				except subprocess.CalledProcessError as err:
					actual = err.stdout;
					returncode = ctypes.c_int32(err.returncode).value;
					if((returncode == 0) and (actual == expected)):
						#print("%-60s%s (E: %d; A: %d)" % (path, "PASSED", expected, actual));
						print("%-60s %s" % (path, "PASSED"));
						tests_passed += 1;
					else:
						if(print_errors): print(err.stdout);
						print("%s %s (E: %d; A: %d)" % (path.ljust(60, '_'), "FAILED", expected, actual));
						tests_failed += 1;
				except subprocess.TimeoutExpired as err:
					print("%s %s (test took longer than %d seconds)" % (path.ljust(60, '_'), "FAILED", test_timeout));
					tests_failed += 1;
				if not(keep_exes): os.remove(file_exe);
			except subprocess.CalledProcessError as err:
				actual = ctypes.c_int32(err.returncode).value;
				if(print_errors): print(err.stdout);
				print("%s %s (solve error: %d)" % (path.ljust(60, '_'), "FAILED", actual));
				tests_failed += 1;
			except subprocess.TimeoutExpired as err:
				print("%s %s (solving took longer than %d seconds)" % (path.ljust(60, '_'), "FAILED", test_timeout));
				tests_failed += 1;
		elif(type == 'invalid'):
			try:
				subprocess.run(compile_cmd, capture_output=True, check=True, encoding="utf-8", timeout=test_timeout);
				
				print("%s %s (no solve error)" % (path.ljust(60, '_'), "FAILED"));
				tests_failed += 1;
			except subprocess.CalledProcessError as err:
				actual = err.stdout;
				returncode = ctypes.c_int32(err.returncode).value;
				if(returncode == expected):
					#print("%-60s%s (E: %d; A: %d)" % (path, "PASSED", expected, returncode));
					print("%-60s %s" % (path, "PASSED"));
					tests_passed += 1;
				else:
					if(print_errors): print(actual);
					print("%s %s (E: %d; A: %d)" % (path.ljust(60, '_'), "FAILED", expected, returncode));
					tests_failed += 1;
			except subprocess.TimeoutExpired as err:
				print("%s %s (solving took longer than %d seconds)" % (path.ljust(60, '_'), "FAILED", test_timeout));
		else:
			print("ERROR: test path did not match format 'group/validity/name.su': ", path);
	print("tests: %d; passed: %d; failed: %d;" % (tests_total, tests_passed, tests_failed));

if __name__ == "__main__":
	main();