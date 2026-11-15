"""Scene-clock choreography. Times are seconds; adjacent moves overlap by 0.24s.

Keep the three existing audible clashes at 19.60, 22.60 and 25.55.
Each tuple is (start, end, clip, contact). Contact maps to 38% of the clip.
No idle clips or held end frames are used during the duel.
"""
OVERLAP = .24
DUEL = {
    'Romeo': [
        (18.80, 20.30, 'MeleeSlash', 19.60),
        (20.06, 21.70, 'MeleeBackhand', 20.72),
        (21.46, 23.18, 'BlockHit', 22.60),
        (22.94, 25.06, 'MeleeCombo', 23.72),
        (24.82, 26.00, 'MeleeDownward', 25.55),
    ],
    'Armoured monster': [
        (18.80, 20.30, 'BlockHit', 19.60),
        (20.06, 21.70, 'BlockHit', 20.72),
        (21.46, 23.18, 'MeleeSlash', 22.60),
        (22.94, 24.16, 'BlockHit', 23.72),
        (23.92, 25.06, 'MeleeBackhand', 24.42),
        (24.82, 26.00, 'MeleeDownward', 25.55),
    ],
}

def smooth(x):
    x = max(0., min(1., x))
    return x*x*(3.-2.*x)

def clip_phase(move, time):
    """Trim neutral anticipation/recovery; retain the contact at its authored time."""
    start, end, _, contact = move
    if time <= contact:
        return .08 + .30 * max(0., (time-start)/(contact-start))
    return min(.90, .38 + .52 * (time-contact)/(end-contact))

def active_moves(who, time):
    return [m for m in DUEL[who] if m[0] <= time <= m[1]]
