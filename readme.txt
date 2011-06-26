== Introduction ==
I originally intended for this mod to be extremely minimalistic, and for personal use only. My original intended changes were just to buff serfdom, protective, and aggressive, and to introduce some features to make automated workers a bit more friendly. The plan was that my mod would be basically the same as the unmodded game, but with a few of the obviously game elements improved. But as I read the game code to learn how to implement my changes, and as I browsed the change-lists of other mods (such as the PIG mod) to see what other minor changes I could make, I started to realise that unmodded BtS isn't anywhere near as planned and polished as I had thought. I use to think that all the mechanics and numbers were a result of careful calculations and thinking and testing, but I learnt that many of the features in the game are completely arbitrary and sometimes obviously unfinished. So the scope of my mod widened dramatically to include pretty much everything that I thought was not as it should be.

I now think of K-Mod as an kind of _unofficial content & balance patch_. The mod is intended to be played as a replacement for standard BtS. Many of the changes are 'under-the-hood' things that a casual player probably wouldn't notice, but I hope the changes will enhance their enjoyment of the game nonetheless.

Here I'll list a few significant features. See changelog.txt for a more complete list.

== Built-in mods ==
 + Actual quotes
 + Better AI (heavily modified)
 + BUG (heavily modified)
 + "unofficial patch" (well known bugfixes)

== New global warming system ==
In standard BtS, global warming is a bit of a joke. It is triggered by the wrong things, you can't really do much to prevent it, and it hits the world in a harsh and unintuitive way. I've completely changed it.

In short, it works like this:
Every point of unhealthiness is counted across the world. If this total is more than some threshold amount then global warming becomes possible, and the likelihood increases as long as the unhealthiness total is above the threshold. There is an environmental advisor which can tell you all the details (on the same screen as the financial advisor).

When global warming strikes, it no longer removes the tile improvement, and it doesn't turn the tile straight into desert, so each strike is far less severe than in original BtS. Also, global warming is more likely to strike cold tiles before hot tiles. eg. The ice caps are likely to melt before your plains get turned into desert.

Positive healthiness (eg. from hospitals) does not reduce the global warming pollution, but a environmentalism and public transport have been changed so that they do reduce unhealthiness rather than increase healthiness.

As global warming becomes more likely, civilizations start to get a happiness penalty which is based on their relative contribution to global pollution.

In standard BtS global warming typically either did nothing, or it completely trashed the world; by contrast, global warming in K-Mod will not trash the world - at least not before someone wins the game - but the effects of global warming will ramp-up towards the end of the game, giving a kind of sense of urgency and tension, this helps build up to a _climactic finish_ at the end of the game.

== New culture system ==
Perceived problems with the old system:
In the old system, plot culture was essentially dominated by 'free culture' from cities. The actual culture output of a city had very little effect other than to increase the culture level of the city. In a culture war (where two civs attempt to push each other's borders back with culture) the culture slider wasn't much use, because the culture output of a city had a relatively minor role on plot culture. 

Two cities would almost always be able to culture press a solo city even when the solo city had much more culture output than the combined total of the two cities. "Culture bombs" did almost nothing, and using spies to spread culture had essentially the same effect on plot culture as a great artist culture bomb! And culture was only useful in border cities (and in the top three culture cities when trying to get a cultural victory).

My solutions:
Plot culture is now primarily determined by the culture output of cities. The 'free' culture has been reduced to almost nothing. (I haven't completely removed free culture because otherwise it would be too easy for civs with the cultural trait to culture press in the early game.) Instant boosts to culture, such as great works and espionage missions, now apply as much plot culture as if the city had produced the culture in the normal way. Finally, plot culture from a city now extends a couple of squares beyond the borders of the city, and so cities don't need to be right on the border of the civilization to contribute to a culture war.

The result is that cultural output now plays a more significant and dynamic role in determining cultural borders. The culture of inner cities is no longer useless, because it will typically extend far enough to help contribute to the culture frontline, and even if it doesn't reach that far it can still contribute via the new 'trade culture' mechanics.

== Many many balance changes ==
I've identified many things that I think were too weak or too strong in original BtS, and I've tried to balance them. But I haven't simply buffed or nerfed things, I've tried to make changes that give each thing its own niche for which it is powerful; so that everything is viable _under the right conditions_, and very few things can be considered 'the best' for all situations. I'm speaking in general terms here because I don't want to try to list every change I've made. But I will give one example:

Serfdom: +1 commerce farms & plantations, -1 commerce from towns (in addition to its original effects).
I've always thought that Serfdom was the most obviously weak civic in the game. In standard BtS, serfdom was very very rarely better than its alternatives. So I wanted to buff it. But I like that slavery is the dominant civic in the early game, and caste system is useful for particular playstyles, and in the late game emancipation gradually (usually) forces most civs out of their otherwise chosen civics...  Everything civic seemed to have a role, and I thought it worked pretty well - except that serfdom was useless. My change to serfdom is only simple, but the effect is that serfdom is now a viable civic for a sigificant part of the game, and it has its own playstyle / strategy type; and it becomes weaker in the late-game as towns become the dominant source of commerce.

Again, see changelog.txt for a more complete list of balance changes.

I hope you enjoy the mod.

 - Karadoc
 