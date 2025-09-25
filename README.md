# PCSX2-COH

This fork of PCSX2 will aim to emulate the arcade PS2 units made by $ony
- `COH-H30000`
- `COH-H31000`
- `COH-H31100`

## The Purpose
just convinience, this will not be usable for anything.  
The code for COH-H models will be kept as an original "parent" fork. since both Namco and Konami made arcades based on the COH-H models  
this fork will focus on what was written above, from here, I will make a fork targetting emulation of the namco System246 and System256 arcades.  
Anyone willing to emulate Konami Python1 can fork this base repo and work from there, this way. we keep a streamlined approach to the actual heart of these two arcades and the differences unrelated to the PS2 inside them can be kept separated

## Things I wont do / Wont happen
> will you Pull request to upstream PCSX2  

No, it is a waste of time trying to do that, supporting arcade PS2s from upstream PCSX2 would mean having two codebases for DEV9 interface, they will most likely not accept that.


> Will this be usable for Namco System147/System148

No, those beasts are quite unique machines, I can't even consider them a PS2 based on how many radical changes they have compared to a COH-H model

> Will this be usable for Konami Python2

No, thats a retail PS2 buried inside an arcade cab. it has nothing to do with real `COH-H` (regardless of Konami Insisting in calling the PS2 inside python2 a `COH-H50000` inside all their manuals/diagrams)

## Common misconceptions

> Magicgate is needed for making arcade PS2 games work

__False.__

`COH-H` models boot process consists on running a bootloader made by $ony from the security dongle (`mc0:boot.bin`), this bootloader is a KELF file, bound to the dongle like retail system updates would. This bootloader setups some stuff and launches a plain ELF file specific to the game. that's the only part of the boot process requiring magicgate, and we can just ignore it and run the plain ELF file that boot.bin would run, just like "play!" does

there is also arcade Syscon and Mechacon constant need for authenticating the Dongle. we don't care of that, since we are emulating those components lol
