proc connect {} {
    set ::cs [socket localhost 8200]
    chan configure $::cs -buffering none -translation binary -encoding binary -blocking 0
    chan event $::cs readable [list client_receive $::cs]
}

proc client_receive {channel} {
    set ::result [chan read $channel]
}

proc send {args} {
    chan puts -nonewline $::cs $args
    vwait ::result
    return $::result
}

proc test {} {
    time {send expr 1+1} 1000
}
