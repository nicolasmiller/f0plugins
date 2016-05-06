//redFrik
//for compability with iphone sc
//mac osx only, requires the RedInfoSms ugen.
//see helpfile RedAccelerometer

RedAccelerometerX : UGen {
	*kr {|minval= 0, maxval= 1, warp= 0, lag= 0.2|
		var trg= Impulse.kr(25, Rand(0, 1));
		var sms= RedInfoSms.kr(trg)[this.axis]*this.mul;
		if(warp==\exponential or:{warp==1}, {
			^Lag.kr(sms.exprange(minval, maxval), lag);
		}, {
			^Lag.kr(sms.range(minval, maxval), lag);
		});
	}
	*categories {^#["UGens>User Interaction"]}
	*axis {^0}
	*mul {^1}
}
RedAccelerometerY : RedAccelerometerX {
	*axis {^1}
	*mul {^1}
}
RedAccelerometerZ : RedAccelerometerX {
	*axis {^2}
	*mul {^-1}
}
