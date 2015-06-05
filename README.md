# [OpenSesame](http://samy.pl/opensesame/)

[OpenSesame](http://samy.pl/opensesame) is a device that can wirelessly open virtually any fixed-code garage door in seconds, exploiting a **new attack** I've discovered on wireless fixed-pin devices. Using a child's toy from Mattel.

##### Follow me on [Twitter](https://twitter.com/samykamkar) or [join my mailing list](http://samy.pl/list/) to hear about future projects and research.

By [@SamyKamkar](https://twitter.com/samykamkar)

**Live demonstration** and full details available in the
<a href="https://www.youtube.com/watch?v=iSSRaIU9_Vc" target="_blank">video</a>:
<a href="https://www.youtube.com/watch?v=iSSRaIU9_Vc" target="_blank"><img src="http://img.youtube.com/vi/iSSRaIU9_Vc/0.jpg?" alt="OpenSesame" width="640" height="480" border="10" /></a>

Released June 4, 2015

**Source code:** [https://github.com/samyk/opensesame](https://github.com/samyk/opensesame)

**Vulnerable Vendors:** The following vendors appear to **directly sell (obviously) insecure products as of June 4, 2015**:

* Nortek / Linear / Multi-Code, [example product](http://www.nortekcontrol.com/product_detail.php?productId=949)
* NSCD/North Shore Commercial Door, [example product](http://www.northshorecommercialdoor.com/pu99gaandgad1.html)

**Previously Vulnerable Vendors:** The following vendors have **old** models that are vulnerable, but current models appear secure from these attacks (or are no longer offered):

* Chamberlain
* Liftmaster
* Stanley
* Delta-3
* Moore-O-Matic

**Prevention:** If you are using a gate or garage which uses "fixed codes", to prevent this type of attack, ensure you upgrade to a system which clearly states that it's using **rolling codes, hopping codes, Security+ or Intellicode**. These are **not** foolproof from attack, but do prevent the OpenSesame attack along with traditional brute forcing attacks. Suggested vendors: current products from LiftMaster and Genie.

**Criminals:** The code I've released is bricked to prevent you from abusing it. It *almost* works, but just not quite, and is released to educate. If you are an expert in RF and microcontrollers, you could fix it, but then you wouldn't need my help in the first place, would you.

-----

# (U) Capabilities

![OpenSesame](http://samy.pl/opensesame/nyan.gif)

**[OpenSesame](http://samy.pl/opensesame)** exploits not only the limited key space of most fixed pin wireless garages and gates, but employs a **new attack** I've discovered reducing the time it takes to open any garage by over 95%. This means most garages will take only seconds to open.

OpenSesame uses the Radica Girltech IM-ME texting toy from Mattel, as it sports all the equipment we need to pull off the attack -- an effective [TI CC1110](http://www.ti.com/product/cc1110-cc1111) sub-GHz RF chip, an LCD display, keyboard, backlight, and more. And it's pink.

This tool builds off of the shoulders of giants, including the original opensesame by Michael Ossmann, IM-ME code & goodfet.cc by Travis Goodspeed, IM-ME LCD reverse engineering by Dave, and effective ideas from Mike Ryan. Additional links and resources included at the end.

Note, this will **not** open garages using rolling codes. Garages with rolling code technology (often called "Intellicode", "Security+", ""hopping codes", etc) are much more secure than fixed-pin garages **but *are* susceptible to other attacks**.

-----

# (U) Primary Issues

*See `The OpenSesame Attack` section below for the **new** attack.*

The simple, less-interesting vulnerability in fixed code systems is the clear fact that their key space is **extremely** limited. For example, a 12-bit (12 binary dip switch) garage/remote supports 12 bits of possible combinations. This is essentially a fixed password that opens your garage. Since it's binary and 12 bits long, that's 2\*\*12, which is 4096 possible combinations.

**A 2-character password on a website is more than twice as hard to solve than to brute force the 12-bit binary dip switch garage.** This is a basic (and sadly long-standing) issue that we exploit, but the exciting attack is in **The OpenSesame Attack** section.

Now in a common garage and clicker, we're going to be using between an 8-12 bit code, and we see a single click sends the same code 5 times, and we see each "bit" takes 2ms to send, with a 2ms wait period per bit after the entire code is sent. So a single 12-bit combination takes (12 bits * 2ms transmit * 2ms wait * 5 times = 240ms). To brute force the entire 8, 9, 10, 11 and 12-bit key space, that's:

(((2 \*\* 12)\*12) + ((2 \*\* 11)\*11) + ((2 \*\* 10)\*10) + ((2 \*\* 9)\*9) + ((2 \*\* 8)\*8)) = 88576 bits

**88576 bits * 4ms * 5 transmits = 1771.52 seconds = 29 minutes**

So it takes 29 minutes to open an (8-12)-bit garage (assuming you know the frequency and baud rate, both of which are pretty common.) If you have to attempt a few different frequencies and baud rates, then the time it takes is a multiple of 29 minutes.

**This is not bad, but we can do better.**

-----

# (U) Initial Reduction

The first attempt at reduction is pretty obvious and is to remove the retransmission. Instead of transmitting the signal 5 times each time, only transmit it once. It's transmitted multiple times to help the receiver detect it in case of interference, but assuming there's no interference or issues receiving, this reduces the time by 5!

**1771.52s / 5 = 354.304 seconds = ~6 minutes**

Nice.

Now while initially testing basic brute forcing on garages, I was chatting with some #ubertooth people, and [mikeryan](https://lacklustre.net/) suggested I remove the wait period between sending each full code and see if I can send each code back-to-back. So instead of sending "111111000000[wait for 12 bits]111111000001", I would send "111111000000111111000001".

This worked, and reduced the entire time to transmit all codes by 50%! Incredible.

**1771.52s / 5 / 2 = 177.152 seconds = ~3 minutes**

**This is not bad, but we can do better.**

-----

# (U) The OpenSesame Attack

![De Bruijn](http://samy.pl/opensesame/db.png)

Here's the kicker. When looking at the data we're sending, we're now sending a continuous stream of bits. For example:

* (code #1) 000000000000
* (code #2) 000000000001
* (code #3) 000000000010 
* (code #4) 000000000011
and so on, which looks like:
**000000000000**000000000001**000000000010**000000000011

The question is, how does the garage receiver look at these bits? What if it's using a bit [shift register](http://en.wikipedia.org/wiki/Shift_register)?

According to Wikipedia:

```In digital circuits, a shift register is a cascade of flip flops, sharing the same clock, in which the output of each flip-flop is connected to the "data" input of the next flip-flop in the chain, resulting in a circuit that shifts by one position the "bit array" stored in it, shifting in the data present at its input and shifting out the last bit in the array, at each transition of the clock input.```

If this is the case, what this means is that if you prepend the real code with *any* amount of bits before or after, the garage won't care and will open.

Let's say our garage pin is 111111000000. If the garage uses a shift register, and we send 13 bits, "**0**111111000000", the garage will first test:
011111100000 (incorrect).

We would assume it will then move onto the next 12 bits (even though there is only one bit left). But no! A shift register only removes 1 bit, then pulls in the next bit.

So the garage actually tests:
011111100000 (incorrect)
(chops off the first bit, then pulls in the next bit)
111111000000 (correct!)

Meaning we sent 13 bits to test **two** 12-bit codes instead of sending a full 24 bits. Incredible!

What's even more beautiful is that since the garage is not clearing an attempted code, a 12 bit code also tests five 8 bit codes, four 9 bit codes, three 10 bit codes, two 11 bit codes, and of course one 12 bit code! As long as we send every 12 bit code, the 8-11 bit codes will all be tested simultaneously.

Now, there must be an algorithm to *efficiently* produce every possible code, with overlap (to exploit the shift register) in as few bits as possible.

**My main man, [De Bruijn](http://en.wikipedia.org/wiki/Nicolaas_Govert_de_Bruijn).**

Nicolaas Govert de Bruijn was a Dutch mathematician who discovered just this, dubbed the [De Bruijn sequence](http://en.wikipedia.org/wiki/De_Bruijn_sequence).

OpenSesame implements this algorithm to produce every possible overlapping sequence of 8-12 bits in the least amount of time. How little time?

To test every 8 through 12 bit possibility:
**((2 \*\* 12) + 11) \* 4ms / 2 = 8214ms = 8.214 seconds**

**We went from 1771 seconds down to 8 seconds. Even our most efficient implementation with the other reductions but without De Bruijn was at 177 seconds, more than 20 times longer. Awesome!**

-----

# (U) Hardware

### IM-ME
The IM-ME from Mattel is a defunct toy no longer produced, but constantly appearing on Amazon and eBay with varying prices from $12 to $100. To my knowledge, much of the reverse engineering of the LCD and keyboard is by [Dave](http://daveshacks.blogspot.com/2010/01/im-me-hacking.html), then tools and more work on it from [Travis Goodspeed](http://travisgoodspeed.blogspot.com/2010/03/im-me-goodfet-wiring-tutorial.html) including support in [GoodFET](http://goodfet.sourceforge.net/clients/goodfetcc/), and more awesome work, including my favorite spectrum analyzer, from [Michael Ossmann](http://ossmann.blogspot.com/2010/03/16-pocket-spectrum-analyzer.html).

It is originally intended as a toy to communicate with friends. It uses the [CC1110](http://www.ti.com/product/cc1110-cc1111), a sub-GHz RF SoC, and sports an LCD display, backlight, keyboard, and is battery powered, all extremely useful for a hacker on the road texting her (or his) friends. Or hacking her (or his) friends. Hopefully both.

Now we could have built our own device, but the beauty of this is that it's all already packaged up for you, inexpensive, and is my favorite color.

### GoodFET
I use Travis Goodspeed's [GoodFET](http://goodfet.sourceforge.net/) device to program the IM-ME as he's built a tool to program it for us!

![GoodFET](http://samy.pl/opensesame/pics/IMG_2757.jpg)

### GIMME
I ghetto-rigged some wire to the test pads and superglued the ends to always connect properly to the GoodFET, but you can also use the [GIMME](http://ossmann.blogspot.com/2012/10/programming-pink-pagers-in-style.html) from Michael Ossmann for a more convenient connector.

![OpenSesame Internals](http://samy.pl/opensesame/pics/back2.jpg)


-----

# (U) Software

### OpenSesame
OpenSesame source code can be obtained in entirety from my github: <https://github.com/samyk/opensesame>

It is originally based off of Michael Ossmann's [opensesame](https://github.com/mossmann/im-me/tree/master/garage) which is specifically built for a fixed code on his garage, and the perfect example for a working OOK/ASK transmitter, handling most of the hardware work for us already. Also, the name was so great I had to use it, I hope Mike doesn't mind.

If you haven't, check out his [spectrum analyzer](http://ossmann.blogspot.com/2010/03/16-pocket-spectrum-analyzer.html) for the IM-ME as I have a secondary IM-ME device loaded with that and it's the perfect portable, inexpensive spectrum analyzer.

### goodfet.cc
As mentioned in the hardware section, we use GoodFET to load the code, and [goodfet.cc](http://goodfet.sourceforge.net/clients/goodfetcc/) specifically to load onto our Chipcon device (TI CC111x = Texas Instruments Chipcon111x)

-----

# (U) Frequencies, Modulations, Encoders

### Frequencies
The immediate assumption is that these fixed pin garages and gates span a wide range of frequencies. For example, [Wikipedia](https://en.wikipedia.org/wiki/Garage_door_opener#Remote_control) suggests these wireless devices span 300MHz - 400MHz, requiring us to send the same signal to 100 additional frequencies. However, after pulling the FCC docs of all of the fixed transmitters I could find, we see only a handful of frequencies are ever used, primarily **300MHz, 310MHz, 315MHz, 318MHz and 390MHz**.

Additionally, most of these receivers lack any [band-pass filter](https://en.wikipedia.org/wiki/Band-pass_filter), allowing a wider range of frequencies to pass through, typically at least covering an additional 2MHz in my testing.

### Modulation
You'll find that virtually all of these transmitters use ASK/OOK to transmit. Additionally, many of the receivers support interoperability by using the same OOK signaling. This can be confirmed by testing several garage door openers, going over the FCC documents for several transmitters, and noting the supported models in various garage door openers.

### Encoders
Here's a list of encoders being used by most of these systems:

PT2262, PT2264, SC2260, CS5211, PT2282, PT2240, eV1527, RT1527, FP527, HS527, SCL1527, MC145026, AX5326, VD5026, SMC926, SMC918, PLC168, HCS300, HCS301, HCS201


-----

# (U) Resources
There are a number of resources and tools, many of which I've learned from and that you can learn more on this and similar topic too. Suggested reading / tools in the area:

* Other projects of mine in the direct area of RF: [KeySweeper](http://samy.pl/keysweeper/) (2.4GHz) and [Digital Ding Dong Ditch](http://samy.pl/dingdong) (sub-GHz)
* Michael Ossmann's [IM-ME spectrum analyzer](http://ossmann.blogspot.com/2010/03/16-pocket-spectrum-analyzer.html)
* Travis Goodspeed's [goodfet.cc / IM-ME wiring](http://travisgoodspeed.blogspot.com/2010/03/im-me-goodfet-wiring-tutorial.html)
* Mike Ryan's [brain](https://lacklustre.net/)
* Michael Ossmann's [HackRF](https://greatscottgadgets.com/hackrf/)
* osmocom's [RTL-SDR](http://sdr.osmocom.org/trac/wiki/rtl-sdr)
* atlas 0f d00m's [rfcat for IM-ME](https://bitbucket.org/atlas0fd00m/rfcat)
* Michael Ossmann's [cc11xx tools](https://github.com/mossmann/cc11xx)
* Dave's [IM-ME LCD hacking](http://daveshacks.blogspot.com/2010/01/im-me-lcd-interface-hacked.html)
* Andrew Nohawk's [Hacking fixed key remotes](http://andrewmohawk.com/2012/09/06/hacking-fixed-key-remotes/)
* Adam Laurie's [You can ring my bell](http://adamsblog.aperturelabs.com/2013/03/you-can-ring-my-bell-adventures-in-sub.html)
* Vegard Haugland's [Hacking garage door remote controllers](http://v3gard.com/2014/12/hacking-garage-door-remote-controllers/)
* TI's [CC111x](http://www.ti.com/product/cc1110-cc1111) [datasheet](http://www.ti.com/lit/gpn/cc1110-cc1111)
* sdcc's [compiler user guide](http://sdcc.sourceforge.net/doc/sdccman.pdf) for the 8051 microcontroller
* the man, the m[ay]themetician, the legend, [De Bruijn](http://en.wikipedia.org/wiki/Nicolaas_Govert_de_Bruijn) and his [sequence](http://en.wikipedia.org/wiki/De_Bruijn_sequence)

-----

# (U) Contact

**Point of Contact:** [@SamyKamkar](https://twitter.com/samykamkar)

You can see more of my projects at <http://samy.pl> or contact me at <code@samy.pl>.

##### Follow me on [Twitter](https://twitter.com/samykamkar) or [join my mailing list](http://samy.pl/list/) to hear about future projects and research.

Thanks!
