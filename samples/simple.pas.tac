__bob:
	_begin_program
	_t5 := d
	_t4 := _t5
	_t3 := _t4
	c := _t3
	_t5 := f
	_t4 := _t5
	_t3 := _t4
	b := _t3
	_t5 := 32
	_t4 := _t5
	_t3 := _t4
	_t1 := _t3 int - 1
	_t2 := _t1 int * 4
	_t8 := 0.000000
	_t7 := _t8
	_t6 := _t7
	c[_t2] := _t6
_L0:
	_t2 := a
	_t14 := a
	_t13 := _t14
	_t12 := _t13
	_t10 := _t12 int - 1
	_t11 := _t10 int * 8
	_t9 := b[_t11]
	_t8 := _t9
	_t6 := inttoreal _t8
	_t1 := _t2 real + _t8
	_t19 := 10
	_t18 := _t19
	_t0 := _t1 <= _t18
	if _t0 goto _L1
	goto _L2
_L1:
	_t5 := a
	_t17 := a
	_t16 := _t17
	_t15 := _t16
	_t13 := _t15 int - 1
	_t14 := _t13 int * 8
	_t12 := b[_t14]
	_t11 := _t12
	_t9 := inttoreal _t11
	_t4 := _t5 real + _t11
	_t3 := _t4
	a := _t3
	_t2 := a
	_t1 := _t2
	_t7 := e
	_t6 := _t7
	_t0 := _t1 = _t6
	if _t0 goto _L3
	goto _L4
_L3:
	_t11 := 32
	_t10 := _t11
	_t9 := _t10
	_t7 := _t9 int - 1
	_t8 := _t7 int * 4
	_t6 := c[_t8]
	_t5 := _t6
	_t4 := _t5
	_t3 := _t4
	e := _t3
	_t5 := 31
	_t4 := _t5
	_t3 := _t4
	_t1 := _t3 int - 1
	_t2 := _t1 int * 4
	_t8 := e
	_t7 := _t8
	_t6 := _t7
	c[_t2] := _t6
	goto _L5
_L4:
	_t11 := 31
	_t10 := _t11
	_t9 := _t10
	_t7 := _t9 int - 1
	_t8 := _t7 int * 4
	_t6 := c[_t8]
	_t5 := _t6
	_t4 := _t5
	_t3 := _t4
	e := _t3
_L5:
	_t5 := 10
	_t4 := _t5
	_t3 := _t4
	_t1 := _t3 int - 1
	_t2 := _t1 int * 4
	_t26 := 3 mod 4
	_t26 := _t26 mod 5
	_t41 := 1.100000
	_t40 := _t41
	_t43 := 32.300000
	_t42 := _t43
	_t39 := _t40 <> _t42
	_t26 := _t26 mod _t39
	_t25 := _t26
	_t24 := _t25
	_t22 := _t24 int - 1
	_t23 := _t22 int * 4
	_t21 := c[_t23]
	_t20 := _t21
	_t19 := _t20
	_t18 := _t19
	_t16 := _t18 int - 1
	_t17 := _t16 int * 4
	_t15 := d[_t17]
	_t14 := _t15
	_t13 := _t14
	_t12 := _t13
	_t10 := _t12 int - 1
	_t11 := _t10 int * 4
	_t9 := c[_t11]
	_t8 := _t9
	_t7 := _t8
	_t6 := _t7
	d[_t2] := _t6
	goto _L0
_L2:
	return
