#!/usr/bin/env python
import sys, os, datetime, time, shutil, tempfile, subprocess
from optparse import OptionParser

usage = "'rethinkdb restore` loads data into a rethinkdb cluster from an archive\n\
  rethinkdb restore FILE -c HOST:PORT [-a KEY] [--force] [-i (DB | DB.TABLE)]..."

def print_restore_help():
    print usage
    print ""
    print "  -h [ --help ]                    print this help"
    print "  FILE                             the archive file to restore data from"
    print "  -c [ --connect ] HOST:PORT       host and port of a rethinkdb node to connect to"
    print "  -a [ --auth ] AUTH_KEY           authorization key for rethinkdb clients"
    print "  --force                          import data even if a table already exists"
    print "  -i [ --import ] (DB | DB.TABLE)  limit restore to the given database or table (may"
    print "                                   be specified multiple times)"

def parse_options():
    parser = OptionParser(add_help_option=False, usage=usage)
    parser.add_option("-c", "--connect", dest="host", metavar="HOST:PORT", default="localhost:28015", type="string")
    parser.add_option("-a", "--auth", dest="auth_key", metavar="KEY", default="", type="string")
    parser.add_option("-i", "--import", dest="tables", metavar="DB | DB.TABLE", default=[], action="append", type="string")
    parser.add_option("--force", dest="force", action="store_true", default=False)
    parser.add_option("-h", "--help", dest="help", default=False, action="store_true")
    (options, args) = parser.parse_args()

    if options.help:
        print_restore_help()
        exit(0)

    # Check validity of arguments
    if len(args) == 0:
        raise RuntimeError("archive to import not specified")
    elif len(args) != 1:
        raise RuntimeError("only one positional argument supported")

    res = { }

    # Verify valid host:port --connect option
    host_port = options.host.split(":")
    if len(host_port) != 2:
        raise RuntimeError("invalid 'host:port' format")
    (res["host"], res["port"]) = host_port

    # Verify valid input file
    res["in_file"] = os.path.abspath(args[0])

    if not os.path.exists(res["in_file"]):
        raise RuntimeError("input file does not exist")

    # Verify valid --import options
    res["dbs"] = []
    res["tables"] = []
    for item in options.tables:
        db_table = item.split(".")
        if len(db_table) == 1:
            res["dbs"].append(db_table[0])
        elif len(db_table) == 2:
            res["tables"].append(tuple(db_table))
        else:
            raise RuntimeError("invalid 'db' or 'db.table' format: %s" % item)

    res["auth_key"] = options.auth_key
    res["force"] = options.force
    return res

def run_rethinkdb_import(options):
    # Find the import script
    import_script = os.path.abspath(os.path.join(os.path.dirname(__file__), "rethinkdb_import.py"))

    if not os.path.exists(import_script):
        raise RuntimeError("could not find import script")

    # Create a temporary directory to store the extracted data
    temp_dir = tempfile.mkdtemp()
    res = -1

    try:
        # TODO: filter stdout/stderr

        tar_args = ["tar", "xzf", options["in_file"], "--force-local", "--strip-components=1", "--wildcards"]
        tar_args.extend(["-C", temp_dir])

        # Only untar the selected tables
        for db in options["dbs"]:
            tar_args.append(os.path.join("*", db))
        for db, table in options["tables"]:
            tar_args.append(os.path.join("*", db, table + ".*"))

        res = subprocess.call(tar_args)
        if res != 0:
            raise RuntimeError("untar of archive failed")

        import_args = [import_script]
        import_args.extend(["--connect", "%s:%s" % (options["host"], options["port"])])
        import_args.extend(["--directory", temp_dir])
        import_args.extend(["--auth", options["auth_key"]])

        for db in options["dbs"]:
            import_args.extend(["--import", db])
        for db, table in options["tables"]:
            import_args.extend(["--import", "%s.%s" % (db, table)])

        if options["force"]:
            import_args.append("--force")

        res = subprocess.call(import_args)
        if res != 0:
            raise RuntimeError("rethinkdb export failed")

    finally:
        shutil.rmtree(temp_dir)

def main():
    try:
        options = parse_options()
        start_time = time.time()
        run_rethinkdb_import(options)
    except RuntimeError as ex:
        print ex
        return 1
    print "Done (%d seconds)" % (time.time() - start_time)
    return 0

if __name__ == "__main__":
    main()