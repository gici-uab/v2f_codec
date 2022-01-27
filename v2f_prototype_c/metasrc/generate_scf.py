#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Script to generate a Software Configuration File (SCF)
"""
__author__ = "Miguel Hernández Cabronero <miguel.hernandez@uab.cat>"
__date__ = "13/09/2020"

import sys
import os
import re
import glob
import hashlib
import csv

# Global description strings
project_name = "Retos Colaboración 2019: Framework para la evaluación de la calidad " \
               "de imágenes para segmentación y clasificación con algoritmos " \
               "de deep learning"

project_acronym = "V2F_UAB"
project_reference_code = "RTC-2019-007434-7"
project_date = "1/8/2021"

project_version = "v1.0"
project_version_branch_type = "main"  # main | parallel
project_version_status = "Validated"
project_version_justifications = [
    "first release"]  # iterable of justification strings
project_tree_structure = "Production tree structure (source files)"
project_delivery_type = "Full"

uab_string = "Universitat Autònoma de Barcelona (UAB)"
author_string = f"Miguel Hernández-Cabronero et al., {uab_string}"
scope_string = f"This software implements a high-quality prototype of a data " \
    f"compression/decompression pair designed for low-complexity scenarios. " \
    f"This is performed using variable to fixed (V2F) code forests " \
    f"in the entropy coding stage of the pipeline."
function_reminder_string = f"This software is designed to perform compression of " \
                           f"earth observation imagery based on V2F forests."

hash_string = "SHA-256"

project_reference_version = "(no previous version delivered)"
master_application_plan_reference = project_reference_code
master_application_plan_version = project_version
master_application_plan_date = project_date

# Path definition
metasource_dir_path, _ = os.path.split(os.path.abspath(__file__))
project_root_path, _ = os.path.split(metasource_dir_path)
build_dir_path = os.path.join(project_root_path, "build")
scf_dir_path = os.path.join(project_root_path, "SCF")
scf_file_path = os.path.join(scf_dir_path, "scf.txt")
# A reference database, i.e., a list of (file_path, hash) tuples, one for each item,
# is saved to current_reference_database_path.
current_reference_database_path = os.path.join(build_dir_path,
                                               "reference_database.csv")
# Path to the reference database generated in the reference version, used to automatically
# generate a list of item changes. If None, it is assumed that this is the first release.
previous_reference_database_path = None

# Script configuration
be_verbose = True

# List of ("section name", write_function) tuples for the registered sections.
# This is populated automatically by the register_section decorator
# in the order in which the decorated functions are defined
sectionname_function_list = []


class SCFGenerator:
    """Class to generate SCFs
    """
    # Maximum number of columns in the output SCF.
    max_width = 80
    # Patterns to identify list lines (and wrap accordingly)
    list_patterns = (r"^(\S+:)", r"^(-)", r"^(\d+.)")

    def __init__(self, scf_file):
        """Initialize a SCF generator.

        :param scf_file: file open for writing where the SCF is to be written
        """
        self.scf_file = scf_file
        self.next_section_number = 0
        self.next_subsection_number = 1
        self.reference_database = {}

    def register_section(section_name):
        """Decorator to register a property as a section
        """

        def decorator_function(function):
            sectionname_function_list.append((section_name, function))
            return function

        return decorator_function

    def write(self, string, indentation=0, use_basic_latex_format=True):
        """Adapt string to the configured format, e.g. trimming as necessary,
        then output to the SCF.

        :param indentation: number of indentation spaces
        """
        # Process assuming latex-like line breaks
        if use_basic_latex_format:
            string = string.replace("\n", " ").replace("\\", "\n")

        # Remove leading and trailing blank lines
        lines = [l.strip() for l in string.splitlines()]
        while lines and not lines[0]:
            lines = lines[1:]
        while lines and not lines[-1]:
            lines = lines[:-1]

        for line in lines:
            last_written_word = ""

            # Lines are split at word boundaries to avoid exceeding max_width
            words = [word.strip() for word in line.split()]

            # Automatic extra indentation is added to split lines
            # that look like a list
            effective_indentation = indentation
            extra_indentation_next_line = 0
            for pattern in self.list_patterns:
                match = re.search(pattern, words[0].strip()) if words else None
                if match:
                    extra_indentation_next_line = len(match.group(0)) + 1
                    break

            current_line_length = 0
            while words:
                word = words[0]
                if use_basic_latex_format:
                    word = word.replace("~", " ")
                words = words[1:]

                # Write indentation if necessary
                if current_line_length <= 0:
                    self.write_raw(" " * effective_indentation)
                    current_line_length += effective_indentation

                # Write current word
                current_word = None
                if current_line_length + len(word) < self.max_width:
                    # Not the last word in the line
                    current_word = f"{word} "
                elif current_line_length + len(word) == self.max_width:
                    # Last word in the line
                    current_word = f"{word}"
                elif current_line_length <= effective_indentation \
                        or any(
                    re.search(pattern, last_written_word) for pattern in
                    self.list_patterns):
                    # Word is exceeds the maximum possible length:
                    # it is output as is with a warning
                    current_word = f"{word}"
                else:
                    # Word did not fit in line. Save for next line unless only
                    # a list indicator has been written so far from this input line
                    words = [word] + words
                    current_word = None
                    current_line_length = self.max_width

                if current_word:
                    self.write_raw(current_word)
                    current_line_length += len(word)
                    last_written_word = current_word

                # Jump to next line if necessary
                if current_line_length >= self.max_width:
                    self.write_raw("\n")
                    current_line_length = -1
                    effective_indentation += extra_indentation_next_line

            if current_line_length != -1:
                self.write_raw("\n")

    def write_raw(self, string):
        """Write string to the SCF without changing it.
        """
        self.scf_file.write(string)

    def write_newline(self, count=1):
        """Write a new line character
        """
        self.write_raw("\n" * count)

    def write_section_separator(self):
        """Write the separator between sections
        """
        self.write_raw("\n" * 2)

    def write_subsection_separator(self):
        """Write the separator between sections
        """
        self.write_raw("\n" * 1)

    def write_section_header(self, section_name):
        """Write a decorated section header and update numeration
        """
        section_name = section_name.title()
        header_text = f"{self.next_section_number}. {section_name}"
        self.write_section_separator()
        self.write(header_text)
        self.write("=" * len(header_text))
        self.next_section_number += 1
        self.next_subsection_number = 1
        if be_verbose:
            print(f"{header_text}")

    def write_subsection_header(self, subsection_name):
        """Write a decorated subsection header and update numeration
        """
        header_text = f"{self.next_section_number - 1}." \
                      f"{self.next_subsection_number} " \
                      f"{subsection_name}"
        self.write_subsection_separator()
        self.write(header_text)
        self.write("-" * len(header_text))
        self.next_subsection_number += 1
        if be_verbose:
            print(f"{header_text}")

    def write_enumeration(self, iterable, starting_number=1, **kwargs):
        """Write a numbered enumeration
        """
        for i, item in enumerate(iterable, start=starting_number):
            self.write(f"{i}. {item}", **kwargs)

    def write_itemization(self, iterable, **kwargs):
        """Write a bullet enumeration
        """
        try:
            kwargs["indentation"] += 4
        except KeyError:
            kwargs["indentation"] = 4
        for item in iterable:
            self.write(f"- {item}", **kwargs)

    def compact_hash_digest(self, hash_digest):
        """Return a compact version of a hash digest
        """
        return f"{hash_digest[:6]}...{hash_digest[-6:]}"

    def write_table(self, header_list, row_list,
                    column_separator=" | ", header_separator_char="-",
                    column_align="<"):
        """Write a table padding fields as necessary.

        :param column_align: < means left aligned, ^ means centered, > means right aligned
        """
        # Calculate table widths
        if len(header_list) > 1:
            assert all(len(header_list) == len(row) for row in row_list)
        assert row_list
        column_widths = [max(len(str(r[i])) for r in row_list)
                         for i in range(len(header_list))]
        column_widths = [max(column_width, len(header_list[i]))
                         for i, column_width in enumerate(column_widths)]

        table_width = sum(column_widths) + len(column_separator) * (
                    len(header_list) - 1)
        row_format_string = column_separator.join(
            f"{{: {column_align}{width}s}}"
            for width in column_widths)

        self.write(header_separator_char * table_width)
        self.write_raw(row_format_string.format(*header_list) + "\n")
        self.write(header_separator_char * table_width)
        for row in row_list:
            self.write_raw(row_format_string.format(*map(str, row)) + "\n")

    def write_file_table(self, file_paths, accept_all_files=True):
        """
        Write a table of files given their paths. Also update the database
        of reference items.

        :param file_paths: list of file paths to show in the table
        :param accept_all_files: if None, only files containing the @file tag
          will be shown in the table (and included in the database)
        """
        file_paths = (file_path
                      for file_path in sorted(list(set(file_paths)))
                      if (os.path.isfile(file_path) and accept_all_files)
                      or self.is_file_in_project(file_path))

        table_rows = []
        for file_path in file_paths:
            file_name = file_path.replace(project_root_path + os.sep, "")
            file_contents = open(file_path, "rb").read()
            file_version = self.get_file_version(file_path=file_path,
                                                 file_contents=file_contents,
                                                 default_version="1.0")
            file_size = os.path.getsize(file_path)
            file_hash = hashlib.sha256(file_contents).hexdigest()

            if file_name in self.reference_database:
                raise ValueError(f"Duplicated file name {file_name}")
            else:
                self.reference_database[file_name] = file_hash

            table_rows.append((
                file_name,
                file_version,
                f"{file_size}",
                self.compact_hash_digest(file_hash)))

        if not table_rows:
            print("WARNING: no entries")
        else:
            self.write_table(
                header_list=["File path", "Version", "Size (bytes)",
                             f"{hash_string}"],
                row_list=table_rows)

    def get_file_version(self, file_path, file_contents=None,
                         default_version=None):
        """Get the version of the given file.

        :param file_contents: contents of the file, or None if not loaded yet
        """
        version = None
        version_patterns = [r"@version\s+(\S+)"]
        if file_contents is not None:
            file_contents = str(file_contents)
        else:
            file_contents = open(file_path, "r").read()

        for version_pattern in version_patterns:

            match = re.search(version_pattern, file_contents)
            if match:
                version = match.group(1)
                break
        if version is None:
            version = default_version
        if version is None:
            raise ValueError(f"Cannot determine version for {file_path}")
        return version

    def is_file_in_project(self, path):
        contents = open(path, "r").read() if os.path.isfile(path) else ""
        return "@file" in contents

    def extract_md_version_date_title(self, md_path):
        """Extract header data from a MD source file

        :return: (version, date, title)
        """
        # Extract data from the header only
        header_separator_count = 0
        header_lines = []
        for line in open(md_path, "r").readlines():
            if line.strip() == "---":
                header_separator_count += 1
            if header_separator_count == 2:
                break
            header_lines.append(line)

        # Extract version
        version = None
        for line in reversed(header_lines):
            match = re.search("[vV]ersion\s*:\s*(\S+)\s*", line)
            if match:
                version = match.group(1)
                break
        assert version

        # Extract date
        date = None
        for line in reversed(header_lines):
            match = re.search("[dD]ate\s*:\s*(\S.*\S)\s*", line)
            if match:
                date = match.group(1).replace(".", "")
                break
        assert date

        # Extract title
        title = None
        for line in reversed(header_lines):
            match = re.search("[tT]itle\s*:\s*(\S.*\S)\s*", line)
            if match:
                title = match.group(1)
                break
        assert title

        return (version, date, title)

    def dump_referece_database(self,
                               output_path=current_reference_database_path):
        """Write a CSV file to output_path with all elements registered in self.database_reference_items
        sorted by file name (including path).
        """
        file_hash_list = ((file_path, file_hash)
                          for file_path, file_hash in
                          self.reference_database.items())
        file_hash_list = sorted(file_hash_list, key=lambda t: t[0])

        with open(output_path, "w") as output_file:
            csv_writer = csv.writer(output_file, delimiter=",")
            for file_name, file_hash in file_hash_list:
                csv_writer.writerow((file_name, file_hash))

        # Verify nothing is lost
        loaded_db = self.load_reference_database(
            input_path=current_reference_database_path)
        for k, v in self.reference_database.items():
            assert loaded_db[k] == v

    def compare_reference_databases(self, old_db, new_db):
        """Compare an old and a new reference databases and return the lists
        of new, deleted, modified keys.

        :return: new_keys, deleted_keys, modified_keys
        """
        new_keys = [k for k in new_db.keys() if k not in old_db]
        deleted_keys = [k for k in old_db.keys() if k not in new_db]
        modified_keys = [k for k in new_db.keys() if
                         k in old_db and old_db[k] != new_db[k]]

        return (new_keys, deleted_keys, modified_keys)

    def load_reference_database(self, input_path):
        """Load a database of reference items from a CSV with the format produced by
        self.dump_reference_database().

        :return: a dictionary indexed by file names (including path) with keys equal to the
          SHA-256 hashes.
        """
        with open(input_path, "r") as input_file:
            csv_reader = csv.reader(input_file, delimiter=",")
            reference_database = {row[0]: row[1] for row in csv_reader}

        return reference_database

    def write_scf(self):
        """Generate the complete SCF
        """
        # SCF Header
        self.write(f"""
            {project_acronym} {project_version} {project_date}\\
            Configuration Description File, produced:\\
            - on: {project_date}\\
            - by: {author_string}\\
            """)

        # Project context section
        self.write_section_header("Project Context")
        self.write_subsection_header("Contents")
        self.write_enumeration(section_name.title()
                               for section_name, _ in
                               sectionname_function_list)
        self.write_subsection_header("References")
        self.write(f"""
            See applicable and reference documents in 
            {master_application_plan_reference} 
            {master_application_plan_version}
            {master_application_plan_date}.
            """)
        self.write_subsection_header("Scope")
        self.write(scope_string)

        # Remaining sections
        for section_name, section_function in sectionname_function_list:
            self.write_section_header(section_name)
            section_function(self=self)

        # Write a simple reference database file for comparison with future versions
        self.dump_referece_database()

    @register_section("Configuration item identification")
    def write_ci_identification(self):
        self.write(f"""
            Item name: {project_acronym}\\
            Product code: {project_reference_code}\\
            Supplier: {uab_string}\\
            \\
            Function reminder:\\
            \\
            {function_reminder_string}
            """)

    @register_section("Version identification")
    def write_version_identification(self):
        self.write(f"""
            Version date: {project_date}\\
            Version number {project_version}\\
        """)

    @register_section("List of software components")
    def write_software_components(self):

        # Source code files
        self.write_subsection_header("Core source code files")
        self.write_file_table(file_paths=glob.glob(os.path.join(
            project_root_path, "src", "*")))

        # Command line tools
        self.write_subsection_header("Command line tools source code files")
        self.write_file_table(file_paths=glob.glob(os.path.join(
            project_root_path, "bin", "*")))

        # Unit testing source
        self.write_subsection_header("Unit testing source code files")
        self.write_file_table(file_paths=glob.glob(os.path.join(
            project_root_path, "test", "*")))

        # Fuzzing files
        self.write_subsection_header("Fuzzer testing source code files")
        self.write_file_table(file_paths=glob.glob(os.path.join(
            project_root_path, "fuzzing", "*")))

        # Metasrc
        self.write_subsection_header("Project scripts")
        self.write_file_table(file_paths=
                              glob.glob(
                                  os.path.join(project_root_path, "metasrc",
                                               "*.sh"))
                              + glob.glob(
                                  os.path.join(project_root_path, "metasrc",
                                               "*.py"))
                              + glob.glob(
                                  os.path.join(project_root_path, "Makefile")))

        # # Crossvalidation scripts
        # self.write_subsection_header("Crossvalidation scripts")
        # self.write_file_table(file_paths=glob.glob(os.path.join(
        #     project_root_path, "crossvalidation", "*sh")))
        # # Crossvalidation binaries
        # self.write_subsection_header("Crossvalidation packages")
        # self.write_file_table(file_paths=glob.glob(os.path.join(
        #     project_root_path, "crossvalidation", "*xz")))

        # Test data
        # Test samples
        self.write_subsection_header("Test data: unit test samples")
        self.write_file_table(file_paths=glob.glob(os.path.join(
            project_root_path, "testdata", "unittest_samples", "**", "*.raw"),
            recursive=True))
        # Fuzzing samples
        self.write_subsection_header("Test data: Fuzzing samples")
        self.write_file_table(file_paths=glob.glob(os.path.join(
            project_root_path, "testdata", "fuzzing_samples", "**", "*"),
            recursive=True))

    # @register_section("Installable Product")
    # def write_installable_product_list(self):
    #     self.write("NOTE: File size and hash depend on the exact version "
    #                "of the assembly tools")
    #
    #     # CLI tools
    #     self.write_subsection_header("Binary command-line tools")
    #     self.write_file_table(file_paths=glob.glob(os.path.join(
    #         project_root_path, "build", "bin", "*")))
    #
    #     # Fuzzers
    #     self.write_subsection_header("Binary fuzzer executables")
    #     self.write_file_table(file_paths=glob.glob(os.path.join(
    #         project_root_path, "build", "fuzzers", "*")))
    #
    #     # Unit tests
    #     self.write_subsection_header("Binary unit test executable")
    #     self.write_file_table(file_paths=glob.glob(os.path.join(
    #         project_root_path, "build", "unittest")))

    @register_section("Documentation")
    def write_documentation(self):
        self.write_subsection_header("Project documentation")
        self.write_file_table(file_paths=glob.glob(os.path.join(project_root_path, "build", "pdf", "*.pdf")))

        self.write_subsection_header("Browsable API Documentation")
        self.write_file_table([
            f for f in glob.glob(os.path.join(
                project_root_path, "build", "doxygen", "html", "**", "*"),
                recursive=True)
            if os.path.isfile(f)])


    @register_section("Open technical events")
    def write_open_technical_events(self):
        self.write_subsection_header("Known bugs")
        self.write("None")

        self.write_subsection_header("Change requests")
        self.write("None")


def print_help():
    print(f"""
Software Configuration File (SCF) generator.

Syntax: {sys.argv[0]} [<output_path>]

Generate the SCF and write it to <output_path>.
If <output_path> is not provided, {scf_file_path} is used by default.""")


if __name__ == '__main__':
    # Select <output_path>
    if len(sys.argv) == 1:
        # No <output_path> was provided. Use default
        os.makedirs(scf_dir_path, exist_ok=True)
        output_path = scf_file_path
    elif len(sys.argv) == 2:
        output_path = sys.argv[1]
    else:
        print_help()
        sys.exit(1)

    if be_verbose:
        print(f"Generating SCF to {output_path}...")
    scf_file = open(output_path, "w")

    # Generate the SCF and write it to <output_path>
    scf_generator = SCFGenerator(scf_file)
    scf_generator.write_scf()

    if be_verbose:
        print("... done!")
