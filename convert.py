import argparse
import fileinput
import os
import subprocess
import sys

parser = argparse.ArgumentParser(description='Convert markdown to asciidoc')
parser.add_argument('--executable', type=str, help='path to kramdoc executable')
args = parser.parse_args()

def findKramdoc():
    def isExe(path):
        return os.path.isfile(path) and os.access(path, os.X_OK)

    if args.executable != None and isExe(args.executable):
        return args.executable

    exe_name = "kramdoc"
    if os.name == "nt":
        exe_name += ".bat"

    for exe_dir in os.environ["PATH"].split(os.pathsep):
        full_path = os.path.join(exe_dir, exe_name)
        if isExe(full_path):
            return full_path

    sys.exit("Could not find kramdoc executable on PATH")

kramdoc_path = findKramdoc()

# Gather files
dir_path = 'modules/ROOT/pages'
markdown_files = []
for root, dirs, files in os.walk(dir_path):
    for file in files:
        if (file.endswith(".md")):
            markdown_files.append(os.path.join(root, file))
        # print(root, dirs, files)

# Note: Nav needs a lot of manual changes, so only build if necessary
build_nav = False

# Convert
nav_entries = []

for dirpath, dirs, files in os.walk("."): 
    if "third_party" in dirpath:
        continue
    if ".github" in dirpath:
        continue
    if "assets" in dirpath:
        continue
    for filename in files:
        fname = os.path.join(dirpath, filename)
        if fname.endswith('.md'):
            adoc_file = fname.replace('.md', '.adoc')
            print("Converting " + fname + "...")
            # subprocess.check_output(kramdoc_path + ' --format=GFM' + ' --output=' + adoc_file + ' --wrap=ventilate ' + fname)
            os.remove(fname)

# for markdown_file in markdown_files:
#     adoc_file = markdown_file.replace('.md', '.adoc')
#     print("Converting " + markdown_file + "...")
#     subprocess.check_output(kramdoc_path + ' --format=GFM' + ' --output=' + adoc_file + ' --wrap=ventilate ' + markdown_file)
#     if build_nav:
#         nav_entries.append(adoc_file)

if build_nav:
    # Build navigation
    last_folder = ''
    nav_file = open("modules/ROOT/nav.adoc", "w")
    for nav_entry in nav_entries:
        link_file = nav_entry.replace("\\", "/")
        link_name = link_file
        link_name = os.path.basename(os.path.normpath(os.path.dirname(link_file)))
        link_name = link_name.replace('_', ' ')
        link_file = link_file.replace(dir_path + '/', '')
        split_str = link_file.split('/')
        # if (last_folder != split_str[0]):
        #     nav_file.write("*")
        if len(split_str) > 2:
            if split_str[0] != last_folder:
                last_folder = split_str[0]
                # nav_file.write("* xref:%s[%s]\n" % (last_folder, 'README.md'))
        nav_file.write("* xref:%s[%s]\n" % (link_file, link_name))
    nav_file.closed