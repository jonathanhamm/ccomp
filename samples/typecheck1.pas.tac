__bob:
	_t5 := 3
	_t4 := _t5
	_t3 := _t4
	a := _t3
	_t5 := a int * 32
	_t5 := _t5 mod 4
	_t5 := _t5 mod 4
	_t4 := _t5
	_t3 := _t4
	_t2 := _t3
	_t1 := _t2
	_t18 := c
	_t17 := _t18
	_t0 := _t1 <> _t17
	if _t0 goto _L3
	goto _L4
_L3:
	_t24 := 234
	_t23 := _t24
	_t22 := _t23
	push _t22
	_t27 := 32
	_t26 := _t27
	_t25 := _t26
	push _t25
	_t30 := 23
	_t29 := _t30
	_t28 := _t29
	push _t28
	_t33 := 23
	_t32 := _t33
	_t31 := _t32
	push _t31
	_t36 := 23
	_t35 := _t36
	_t34 := _t35
	push _t34
	call __bob_lolwut
	goto _L5
_L4:
_L6:
	_t2 := a
	_t1 := _t2
	_t7 := c
	_t6 := _t7
	_t0 := _t1 < _t6
	if _t0 goto _L7
	goto _L8
_L7:
	_t5 := a int * 32
	_t15 := inttoreal _t5
	_t5 := _t15 real * c
	_t5 := a real * 32
	_t4 := _t5
	_t3 := _t4
	a := _t3
	_t5 := a int * 23
	_t5 := _t5 int * 23
	_t18 := inttoreal 4
	_t5 := _t5 / _t18
	_t24 := inttoreal 32
	_t5 := _t5 real * _t24
	_t26 := inttoreal 23
	_t5 := _t5 / _t26
	_t5 := _t5 real * 23
	_t5 := a real * 23
	_t4 := _t5
	_t3 := _t4
	c := _t3
	goto _L6
_L8:
_L5:
	_t11 := 4
	_t10 := _t11
	_t9 := _t10
	_t7 := _t9 int - 0
	_t8 := _t7 int * 4
	_t6 := a[_t8]
	_t5 := _t6
	_t4 := _t5
	_t3 := _t4
	c := _t3
	return
	 
__bob_lolwut:
	_beginfunc
	_t2 := a
	_t1 := _t2
	_t7 := b
	_t6 := _t7
	_t0 := _t1 <> _t6
	if _t0 goto _L0
	goto _L1
_L0:
	_t13 := a
	_t12 := _t13
	_t11 := _t12
	push _t11
	_t19 := b
	_t18 := _t19
	_t17 := _t18
	push _t17
	_t25 := c
	_t24 := _t25
	_t23 := _t24
	push _t23
	call __bob_lolwut
	goto _L2
_L1:
_L2:
	_t2 := a
	_t1 := _t2
	_t0 := _t1
	push _t0
	_t8 := c
	_t7 := _t8
	_t6 := _t7
	push _t6
	_t14 := b
	_t13 := _t14
	_t12 := _t13
	push _t12
	call __bob_lolwut
	_t5 := a int * a
	_t18 := a
	_t4 := _t5 int + _t18
	_t3 := _t4
	_t1 := _t3 int -14
	_t2 := _t1 int * 8
	_t24 := 333 int * 34
	_t23 := _t24
	_t22 := _t23
	kimjongil[_t2] := _t22
	_t5 := c real * b
	_t19 := inttoreal a
	_t5 := _t5 real * _t19
	_t5 := _t5 real * c
	_t31 := inttoreal 3
	_t5 := _t5 / _t31
	_t4 := _t5
	_t3 := _t4
	b := _t3
	_t5 := bosstweed
	_t4 := _t5
	_t3 := _t4
	kimjongil := _t3
	call __bob_lolwut
	_t17 := inttoreal a
	_t11 := _t17 real * b
	_t10 := _t11
	_t9 := _t10
	_t7 := _t9 int - 14
	_t8 := _t7 int * 8
	_t6 := kimjongil[_t8]
	_t5 := _t6
	_t4 := _t5
	_t3 := _t4
	c := _t3
	return
