import unittest
from act2_flow_plan import DUEL, active_moves, clip_phase

class FlowPlanTest(unittest.TestCase):
    def test_continuous_combat_without_idle_or_frozen_endpoints(self):
        for who in DUEL:
            for frame in range(564,780):
                moves=active_moves(who,frame/30)
                self.assertTrue(moves,(who,frame))
                self.assertLessEqual(len(moves),2)
                for move in moves:
                    self.assertNotIn('Idle',move[2])
                    self.assertGreaterEqual(clip_phase(move,frame/30),.08-1e-6)
                    self.assertLessEqual(clip_phase(move,frame/30),.90+1e-6)

    def test_existing_clashes_remain_paired(self):
        for time in [19.60,22.60,25.55]:
            for who in DUEL:
                moves=[m for m in active_moves(who,time) if m[3]==time]
                self.assertEqual(len(moves),1)
                self.assertAlmostEqual(clip_phase(moves[0],time),.38)

    def test_moves_progress_and_keep_death_at_26(self):
        for moves in DUEL.values():
            self.assertEqual(moves[-1][1],26)
            for move in moves:
                self.assertLess(move[0],move[3])
                self.assertLess(move[3],move[1])
                phases=[clip_phase(move,move[0]+(move[1]-move[0])*i/100) for i in range(101)]
                self.assertEqual(phases,sorted(phases))

if __name__=='__main__':unittest.main()
