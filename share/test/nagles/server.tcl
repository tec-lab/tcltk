proc create {} {
    set ::ss [socket -server server_register 8200]
}

proc server_register {channel clientaddr clientport} {
    chan configure $channel -buffering none -translation binary -encoding binary -blocking 0
    chan event $channel readable [list server_receive $channel]
}

proc server_receive {channel} {
    set data [chan read $channel]
    catch $data result
    chan puts -nonewline $channel $result
}
