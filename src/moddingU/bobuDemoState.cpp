#include "Game/SingleGame.h"
#include "mod.h"

namespace Game {
namespace SingleGame {

// registerBobuDemoState__Q24Game10SingleGameFPQ34Game10SingleGame3FSM
// in singleGameSection.s
void registerBobuDemoState(FSM* fsm) { fsm->registerState(new BobuDemoState); }

// transitToBobuDemoState__Q24Game10SingleGameFPQ34Game10SingleGame15CaveResultStatePQ24Game17SingleGameSection
// in singleGS_CaveResult.s
void transitToBobuDemoState(CaveResultState* state, SingleGameSection* section)
{
	state->transit(section, SGS_BobuDemo, nullptr);
	// OSReport("movie queued: %i, state status: %i\n", mod::isBobuMovieQueued, state->_12);
}

BobuDemoState::BobuDemoState()
    : State(SGS_BobuDemo)
{
	mCurrentHeap = nullptr;
	mMovieHeap   = nullptr;
}

void BobuDemoState::init(SingleGameSection*, StateArg*)
{
	mCurrentHeap = JKRHeap::sCurrentHeap;
	mMovieHeap   = JKRExpHeap::create(mCurrentHeap->getFreeSize(), mCurrentHeap, true);
	mMovieHeap->becomeCurrentHeap();

	mThpPlayer = new THPPlayer;
	mThpPlayer->init(nullptr);
	mThpPlayer->load(THPPlayer::CRIME);
	mIsMoviePlaying = false;
}

void BobuDemoState::exec(SingleGameSection* section)
{
	if (!mMovieHeap) {
		return;
	}

	mThpPlayer->update();
	if (!mIsMoviePlaying) {
		if (mThpPlayer->isFinishLoading()) {
			mThpPlayer->play();
			mIsMoviePlaying = true;
		}
	} else if (mThpPlayer->isFinishPlaying()) {
		// left via geyser, not in a cave, dont clear heap, no sublevel count increment
		LoadArg arg(1, false, true, false);
		section->loadMainMapSituation();
		transit(section, SGS_Load, &arg);
	}
}

void BobuDemoState::draw(SingleGameSection*, Graphics& gfx)
{
	if (mMovieHeap) {
		mThpPlayer->draw(gfx);
	}
}

void BobuDemoState::cleanup(SingleGameSection*)
{
	mMovieHeap->freeAll();
	mMovieHeap->destroy();
	mMovieHeap = nullptr;

	mCurrentHeap->becomeCurrentHeap();
}

} // namespace SingleGame
} // namespace Game
