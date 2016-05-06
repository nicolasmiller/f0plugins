//redFrik
RedInfoBat : UGen {
	*kr {|trig= 0|
		^this.multiNew('control', trig)
	}
	*categories {^#["UGens>User interaction"]}
}
RedInfoLmu : MultiOutUGen {
	*kr {|trig= 0|
		^this.multiNew('control', trig)
	}
	init {|... theInputs|
		inputs= theInputs;
		^this.initOutputs(2, rate);
	}
	*categories {^#["UGens>User interaction"]}
}
RedInfoSms : MultiOutUGen {
	*kr {|trig= 0|
		^this.multiNew('control', trig)
	}
	init {|... theInputs|
		inputs= theInputs;
		^this.initOutputs(3, rate);
	}
	*categories {^#["UGens>User interaction"]}
}
RedInfoSms2 : RedInfoSms {}
RedInfoSms3 : RedInfoSms {}
