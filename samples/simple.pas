program bob (input, output);
var a : integer;
var b : array[1..10] of real;
var c : array[1..10] of integer;
var d : array[1..11] of integer;
var e : real;
var f : array[1..10] of real;
begin
    c := d;
    b := f;
    c[32] := 32.32E-23;
    while a + b[a] <= 10 do
    begin
        a := a + b[a];
        if a = e then
            begin
                e := c[32];
                c[31] := e
            end
        else
            e := c[31];
        d[10] := c[d[c[3 mod 4 mod 5 mod (1.1 <> 32.3)]]]
    end
end.
