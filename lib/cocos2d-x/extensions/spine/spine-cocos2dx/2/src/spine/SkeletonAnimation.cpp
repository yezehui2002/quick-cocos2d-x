/******************************************************************************
 * Spine Runtimes Software License
 * Version 2.1
 *
 * Copyright (c) 2013, Esoteric Software
 * All rights reserved.
 *
 * You are granted a perpetual, non-exclusive, non-sublicensable and
 * non-transferable license to install, execute and perform the Spine Runtimes
 * Software (the "Software") solely for internal use. Without the written
 * permission of Esoteric Software (typically granted by licensing Spine), you
 * may not (a) modify, translate, adapt or otherwise create derivative works,
 * improvements of the Software or develop new applications using the Software
 * or (b) remove, delete, alter or obscure any trademarks or any copyright,
 * trademark, patent or other intellectual property or proprietary rights
 * notices on or in the Software, including any copy thereof. Redistributions
 * in binary or source form must include this license and terms.
 *
 * THIS SOFTWARE IS PROVIDED BY ESOTERIC SOFTWARE "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ESOTERIC SOFTARE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include "SkeletonAnimation.h"
#include <spine/spine-cocos2dx.h>
#include <spine/extension.h>
#include <algorithm>

#include "CCLuaEngine.h"

USING_NS_CC;
using std::min;
using std::max;
using std::vector;

namespace spine {
    
    void animationCallback (spAnimationState* state, int trackIndex, spEventType type, spEvent* event, int loopCount) {
        ((SkeletonAnimation*)state->rendererObject)->onAnimationStateEvent(trackIndex, type, event, loopCount);
    }
    
    void trackEntryCallback (spAnimationState* state, int trackIndex, spEventType type, spEvent* event, int loopCount) {
        ((SkeletonAnimation*)state->rendererObject)->onTrackEntryEvent(trackIndex, type, event, loopCount);
    }
    
    typedef struct _TrackEntryListeners {
        StartListener startListener;
        EndListener endListener;
        CompleteListener completeListener;
        EventListener eventListener;
    } _TrackEntryListeners;
    
    static _TrackEntryListeners* getListeners (spTrackEntry* entry) {
        if (!entry->rendererObject) {
            entry->rendererObject = NEW(spine::_TrackEntryListeners);
            entry->listener = trackEntryCallback;
        }
        return (_TrackEntryListeners*)entry->rendererObject;
    }
    
    void disposeTrackEntry (spTrackEntry* entry) {
        if (entry->rendererObject) FREE(entry->rendererObject);
        _spTrackEntry_dispose(entry);
    }
    
    //
    
    SkeletonAnimation* SkeletonAnimation::createWithData (spSkeletonData* skeletonData) {
        SkeletonAnimation* node = new SkeletonAnimation(skeletonData);
        node->autorelease();
        return node;
    }
    
    SkeletonAnimation* SkeletonAnimation::createWithFile (const char* skeletonDataFile, spAtlas* atlas, float scale) {
        SkeletonAnimation* node = new SkeletonAnimation(skeletonDataFile, atlas, scale);
        node->autorelease();
        return node;
    }
    
    SkeletonAnimation* SkeletonAnimation::createWithFile (const char* skeletonDataFile, const char* atlasFile, float scale) {
        SkeletonAnimation* node = new SkeletonAnimation(skeletonDataFile, atlasFile, scale);
        node->autorelease();
        return node;
    }
  
    
    SkeletonAnimation::SkeletonAnimation (spSkeletonData* skeletonData,spAtlas* atlas)
	:SkeletonRenderer(skeletonData, atlas ) {
		m_scritpHandler = 0;
		initialize();
    }
    
    void SkeletonAnimation::initialize () {
        ownsAnimationStateData = true;
        state = spAnimationState_create(spAnimationStateData_create(skeleton->data));
        state->rendererObject = this;
        state->listener = animationCallback;
        
        _spAnimationState* stateInternal = (_spAnimationState*)state;
        stateInternal->disposeTrackEntry = disposeTrackEntry;
    }
    
    
    SkeletonAnimation::SkeletonAnimation (spSkeletonData *skeletonData)
    : SkeletonRenderer(skeletonData) {
        m_scritpHandler = 0;
        initialize();
    }
    
    SkeletonAnimation::SkeletonAnimation (const char* skeletonDataFile, spAtlas* atlas, float scale)
    : SkeletonRenderer(skeletonDataFile, atlas, scale) {
        initialize();
    }
    
    SkeletonAnimation::SkeletonAnimation (const char* skeletonDataFile, const char* atlasFile, float scale)
    : SkeletonRenderer(skeletonDataFile, atlasFile, scale) {
        m_scritpHandler = 0;
        initialize();
    }
    
    
    
    SkeletonAnimation::~SkeletonAnimation () {
        if (ownsAnimationStateData) spAnimationStateData_dispose(state->data);
        spAnimationState_dispose(state);
    }
    
    void SkeletonAnimation::update (float deltaTime) {
        super::update(deltaTime);
        
        deltaTime *= timeScale;
        spAnimationState_update(state, deltaTime);
        spAnimationState_apply(state, skeleton);
        spSkeleton_updateWorldTransform(skeleton);
    }
    
    void SkeletonAnimation::setAnimationStateData (spAnimationStateData* stateData) {
        CCAssert(stateData, "stateData cannot be null.");
        
        if (ownsAnimationStateData) spAnimationStateData_dispose(state->data);
        spAnimationState_dispose(state);
        
        ownsAnimationStateData = false;
        state = spAnimationState_create(stateData);
        state->rendererObject = this;
        state->listener = animationCallback;
    }
    
    void SkeletonAnimation::setMix (const char* fromAnimation, const char* toAnimation, float duration) {
        spAnimationStateData_setMixByName(state->data, fromAnimation, toAnimation, duration);
    }
    
    spTrackEntry* SkeletonAnimation::setAnimation (int trackIndex, const char* name, bool loop) {
        spAnimation* animation = spSkeletonData_findAnimation(skeleton->data, name);
        if (!animation) {
            CCLog("Spine: Animation not found: %s", name);
            return 0;
        }
        return spAnimationState_setAnimation(state, trackIndex, animation, loop);
    }
    
    spTrackEntry* SkeletonAnimation::addAnimation (int trackIndex, const char* name, bool loop, float delay) {
        spAnimation* animation = spSkeletonData_findAnimation(skeleton->data, name);
        if (!animation) {
            CCLog("Spine: Animation not found: %s", name);
            return 0;
        }
        return spAnimationState_addAnimation(state, trackIndex, animation, loop, delay);
    }
    
    spTrackEntry* SkeletonAnimation::getCurrent (int trackIndex) {
        return spAnimationState_getCurrent(state, trackIndex);
    }
    
    void SkeletonAnimation::clearTracks () {
        spAnimationState_clearTracks(state);
    }
    
    void SkeletonAnimation::clearTrack (int trackIndex) {
        spAnimationState_clearTrack(state, trackIndex);
    }
    
    void SkeletonAnimation::onAnimationStateEvent (int trackIndex, spEventType type, spEvent* event, int loopCount) {
        char *strType = NULL;
        switch (type) {
            case SP_ANIMATION_START:
                strType = "START";
                if (startListener) startListener(trackIndex);
                break;
            case SP_ANIMATION_END:
                strType = "END";
                if (endListener) endListener(trackIndex);
                break;
            case SP_ANIMATION_COMPLETE:
                strType = "COMPLETE";
                if (completeListener) completeListener(trackIndex, loopCount);
                break;
            case SP_ANIMATION_EVENT:
                strType = "EVENT";
                if (eventListener) eventListener(trackIndex, event);
                break;
        }
        
        //<调lua接口
        //<执行lua脚本回调
        if (m_scritpHandler)
        {
			//<调lua脚本进行消息封装
			lua_State* state = CCLuaEngine::defaultEngine()->getLuaStack()->getLuaState();	//<需要使用LuaEngine的state,否则向luaEngine中注册的自定义类均无法使用
            
			//<压参
			CCLuaEngine::defaultEngine()->getLuaStack()->pushInt(-1);	 //<多参数必须在开头压空(栈尾..)
			CCLuaEngine::defaultEngine()->getLuaStack()->pushInt(trackIndex);
			CCLuaEngine::defaultEngine()->getLuaStack()->pushString(strType);
			CCLuaEngine::defaultEngine()->getLuaStack()->pushString((*(this->state->tracks))->animation->name); //<当前执行的动画名,tracks为数组,如果不做混合那么只需取第一个
            
			CCLuaValueDict dict;
			if (event != 0)		//<将spEvent结构转为表压入
			{
				dict["data_fVal"] = CCLuaValue::floatValue(event->data->floatValue);
				dict["data_iVal"] = CCLuaValue::intValue(event->data->intValue);
				dict["data_name"] = CCLuaValue::stringValue(event->data->name);
				dict["data_strVal"] = CCLuaValue::stringValue(event->data->stringValue);
				dict["fVal"] = CCLuaValue::floatValue(event->floatValue);
				dict["iVal"] = CCLuaValue::intValue(event->intValue);
				dict["strVal"] = CCLuaValue::stringValue(event->stringValue);
			}
            
			CCLuaEngine::defaultEngine()->getLuaStack()->pushCCLuaValueDict(dict);
			CCLuaEngine::defaultEngine()->getLuaStack()->pushInt(loopCount);
            
			int ret = CCLuaEngine::defaultEngine()->getLuaStack()->executeFunctionByHandler(m_scritpHandler, 6);
			CCLuaEngine::defaultEngine()->getLuaStack()->clean();
        }
    }
    
    void SkeletonAnimation::setScriptHandler(int handler){
        m_scritpHandler = handler;
    }
    
    //<根据Slot获取包围框
    CCRect SkeletonAnimation::boundingBoxOfSlot(const char* slotName)
    {
		//<在drawOrder中检索指定slot
		spSlot **pOrder = this->skeleton->drawOrder;			//[18]
		spSlot* target = this->findSlot(slotName), *slot = *pOrder;
		while(slot != target)
		{
			pOrder =pOrder + 1;
			slot = *pOrder;
		}
        
		spRegionAttachment* attachment = (spRegionAttachment*)slot->attachment;
		void* texture = (CCTexture2D*)((spAtlasRegion*)attachment->rendererObject)->page->rendererObject;
        
		//<获取region顶点
		float worldVertices[8] = {0,0,0,0,0,0,0,0};
		CCPoint points[4];
		spRegionAttachment* attach = (spRegionAttachment*)slot->attachment;
		spRegionAttachment_computeWorldVertices(attach, slot->skeleton->x, slot->skeleton->y, slot->bone, worldVertices);
		points[0] = ccp(worldVertices[0], worldVertices[1]);
		points[1] = ccp(worldVertices[2], worldVertices[3]);
		points[2] = ccp(worldVertices[4], worldVertices[5]);
		points[3] = ccp(worldVertices[6], worldVertices[7]);
        
		CCRect rect = CCRectMake(points[0].x, points[0].y, 0, 0);
		for (int i = 0; i<4; i++)	//<确定原点
		{
			if ( points[i].x < rect.origin.x ) {	rect.origin.x = points[i].x; }
			if ( points[i].y < rect.origin.y ) {	rect.origin.y = points[i].y; }
		}
        
		for (int i = 0; i<4; i++)
		{
			if ( points[i].x -  rect.origin.x > rect.size.width  ) {	rect.size.width = points[i].x - rect.origin.x; }
			if ( points[i].y -  rect.origin.y > rect.size.height ) {	rect.size.height = points[i].y - rect.origin.y; }
		}
        
		//<y轴转向时按照origin不动,width翻转的原则
		//rect.size.width = rect.size.width * getScaleX();
		if (getScaleX() == -1)
		{
			rect.origin.x += rect.size.width;
		}
		rect.origin = this->convertToWorldSpace(rect.origin);
        
		//rect.origin.x = rect.origin.x *getScaleX();
        
		//<构造顶点,矩形
		CCNode *pRoot = this->getParent();
		if (!pRoot) { CCLOG("spine root node not found!"); return rect; }
        
        /*		CCSprite *sp = CCSprite::create("../res/Pnts.png", rect);
         sp->setAnchorPoint(ccp(0,0));
         sp->setPosition(ccp(rect.origin.x, rect.origin.y));
         pRoot->getParent()->getParent()->removeChildByTag(133);
         pRoot->getParent()->getParent()->addChild(sp,100, 133);
         */
		return rect;
        
		/*	直接使用编辑的boundingbox的方法
         spSlot* slot = skeletonNode->findSlot("left hand");
         spBoundingBoxAttachment* box = SUB_CAST(spBoundingBoxAttachment, slot->attachment);
         size_t count = box->verticesCount;
         spSlot *pOd = skeletonNode->skeleton->drawOrder[1];*/
        
		/*
         spine::SkeletonAnimation* skeletonNode = dynamic_cast<spine::SkeletonAnimation*>(skNode->getChildByTag(1));
         if (skeletonNode)
         {
         spSlot* slot = skeletonNode->findSlot("fist");
         spBoundingBoxAttachment* box = SUB_CAST(spBoundingBoxAttachment, slot->attachment);
         size_t count = box->verticesCount;
         
         CCSize size;
         CCPoint orign = ccp(9999,9999), rt = ccp(-9999,-9999);
         for(size_t i = 0; i < count; i+=2) //<(x,y)
         {
         printf("x: %f, y %f",box->vertices[i] , box->vertices[i+1]);
         //<process x
         if ( box->vertices[i] < orign.x )
         {
         orign.x = box->vertices[i];
         }
         else if( box->vertices[i] > rt.x )
         {
         rt.x = box->vertices[i];
         }
         
         //<process y
         if ( box->vertices[i+1] < orign.y )
         {
         orign.y = box->vertices[i+1];
         }
         else if( box->vertices[i+1] > rt.y )
         {
         rt.y = box->vertices[i+1];
         }
         }
         CCRect rect = CCRectMake(orign.x, orign.y, rt.x - orign.x, rt.y - orign.y );
         //这样可以获取到定点集(似乎坐标为local),接下来: 转为最大的boundingBox..
         CCSprite *sp = CCSprite::create("../res/Pnts.png");
         sp->setPosition(ccp(orign.x, orign.y));
         sp->setAnchorPoint(ccp(0,0));
         skeletonNode->addChild(sp);
         return rect;
         }*/
    }
    
    void SkeletonAnimation::onTrackEntryEvent (int trackIndex, spEventType type, spEvent* event, int loopCount) {
        spTrackEntry* entry = spAnimationState_getCurrent(state, trackIndex);
        if (!entry->rendererObject) return;
        _TrackEntryListeners* listeners = (_TrackEntryListeners*)entry->rendererObject;
        switch (type) {
            case SP_ANIMATION_START:
                if (listeners->startListener) listeners->startListener(trackIndex);
                break;
            case SP_ANIMATION_END:
                if (listeners->endListener) listeners->endListener(trackIndex);
                break;
            case SP_ANIMATION_COMPLETE:
                if (listeners->completeListener) listeners->completeListener(trackIndex, loopCount);
                break;
            case SP_ANIMATION_EVENT:
                if (listeners->eventListener) listeners->eventListener(trackIndex, event);
                break;
        }
    }
    
    void SkeletonAnimation::setStartListener (spTrackEntry* entry, StartListener listener) {
        getListeners(entry)->startListener = listener;
    }
    
    void SkeletonAnimation::setEndListener (spTrackEntry* entry, EndListener listener) {
        getListeners(entry)->endListener = listener;
    }
    
    void SkeletonAnimation::setCompleteListener (spTrackEntry* entry, CompleteListener listener) {
        getListeners(entry)->completeListener = listener;
    }
    
    void SkeletonAnimation::setEventListener (spTrackEntry* entry, spine::EventListener listener) {
        getListeners(entry)->eventListener = listener;
    }
    
}
