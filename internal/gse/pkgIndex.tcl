package ifneeded gse 1.4 {
    package provide gse 1.4
    namespace eval gse {
        variable tcl_release "%tcl_release%"
        variable build_os "%build_os%"
        variable build_kernelname "%build_kernelname%"
        variable build_kernelrelease "%build_kernelrelease%"
        variable build_kernelversion "%build_kernelversion%"
        variable build_nodename "%build_nodename%"
        variable build_machine "%build_machine%"
        variable build_date "%build_date%"
        variable git_hash "%git_hash%"
        variable gcc_version "%gcc_version%"
    }
}
