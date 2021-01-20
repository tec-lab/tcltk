canvas .c 
pack .c 

set ::blah {} 
proc create {} { 
  catch {destroy .c.e} 
  entry .c.e -textvariable ::blah 
  pack .c.e 
  focus .c.e 
  .c.e icursor end 
  after 100 ::create 
} 
create 
