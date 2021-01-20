# 2018 May 19
#
# The author disclaims copyright to this source code.  In place of
# a legal notice, here is a blessing:
#
#    May you do good and not evil.
#    May you find forgiveness for yourself and forgive others.
#    May you share freely, never taking more than you give.
#
#***********************************************************************
#

package require sqlite3
package require Pgtcl

set db [pg_connect -conninfo "dbname=postgres user=postgres password=postgres"]
sqlite3 sqlite ""

proc execsql {sql} {

  set sql [string map {{WITHOUT ROWID} {}} $sql]

  set lSql [list]
  set frag ""
  while {[string length $sql]>0} {
    set i [string first ";" $sql]
    if {$i>=0} {
      append frag [string range $sql 0 $i]
      set sql [string range $sql $i+1 end]
      if {[sqlite complete $frag]} {
        lappend lSql $frag
        set frag ""
      }
    } else {
      set frag $sql
      set sql ""
    }
  }
  if {$frag != ""} {
    lappend lSql $frag
  }
  #puts $lSql

  set ret ""
  set nChar 0
  foreach stmt $lSql {
    set res [pg_exec $::db $stmt]
    set err [pg_result $res -error]
    if {$err!=""} { error $err }

    for {set i 0} {$i < [pg_result $res -numTuples]} {incr i} {
      set t [pg_result $res -getTuple $i]
      set nNew [string length $t]
      if {$nChar>0 && ($nChar+$nNew+3)>75} {
        append ret "\n  "
        set nChar 0
      } else {
        if {$nChar>0} {
          append ret "   "
          incr nChar 3
        }
      }
      incr nChar $nNew
      append ret $t
    }
    pg_result $res -clear
  }

  set ret
}

proc execsql_test {tn sql} {
  set res [execsql $sql]
  set sql [string map {string_agg group_concat} $sql]
  # set sql [string map [list {NULLS FIRST} {}] $sql]
  # set sql [string map [list {NULLS LAST} {}] $sql]
  puts $::fd "do_execsql_test $tn {"
  puts $::fd "  [string trim $sql]"
  puts $::fd "} {$res}"
  puts $::fd ""
}

proc errorsql_test {tn sql} {
  set rc [catch {execsql $sql} msg]
  if {$rc==0} {
    error "errorsql_test SQL did not cause an error!"
  }
  set msg [lindex [split [string trim $msg] "\n"] 0]
  puts $::fd "# PG says $msg"
  set sql [string map {string_agg group_concat} $sql]
  puts $::fd "do_test $tn { catch { execsql {"
  puts $::fd "  [string trim $sql]"
  puts $::fd "} } } 1"
  puts $::fd ""
}

# Same as [execsql_test], except coerce all results to floating point values
# with two decimal points.
#
proc execsql_float_test {tn sql} {
  set F "%.4f"
  set T 0.0001
  set res [execsql $sql]
  set res2 [list]
  foreach r $res { 
    if {$r != ""} { set r [format $F $r] }
    lappend res2 $r
  }

  set sql [string trim $sql]
puts $::fd [subst -nocommands {
do_test $tn {
  set myres {}
  foreach r [db eval {$sql}] {
    lappend myres [format $F [set r]]
  }
  set res2 {$res2}
  set i 0
  foreach r [set myres] r2 [set res2] {
    if {[set r]<([set r2]-$T) || [set r]>([set r2]+$T)} {
      error "list element [set i] does not match: got=[set r] expected=[set r2]"
    }
    incr i
  }
  set {} {}
} {}
}]
}

proc start_test {name date} {
  set dir [file dirname $::argv0]
  set output [file join $dir $name.test]
  set ::fd [open $output w]
puts $::fd [string trimleft "
# $date
#
# The author disclaims copyright to this source code.  In place of
# a legal notice, here is a blessing:
#
#    May you do good and not evil.
#    May you find forgiveness for yourself and forgive others.
#    May you share freely, never taking more than you give.
#
#***********************************************************************
# This file implements regression tests for SQLite library.
#

####################################################
# DO NOT EDIT! THIS FILE IS AUTOMATICALLY GENERATED!
####################################################
"]
  puts $::fd {set testdir [file dirname $argv0]}
  puts $::fd {source $testdir/tester.tcl}
  puts $::fd "set testprefix $name"
  puts $::fd ""
}

proc -- {args} {
  puts $::fd "# $args"
}

proc ========== {args} {
  puts $::fd "#[string repeat = 74]"
  puts $::fd ""
}

proc finish_test {} {
  puts $::fd finish_test
  close $::fd
}

proc ifcapable {arg} {
   puts $::fd "ifcapable $arg { finish_test ; return }"
}
