#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <alphalaneous.alphas-ui-pack/include/API.hpp>

#ifdef GEODE_IS_WINDOWS
#include <Windows.h>
#endif

using namespace geode::prelude;
using namespace alpha::prelude;

struct NodePhysics {
    CCPoint pos;
    CCPoint velocity;
    CCPoint originalPos;
    bool initialized = false;
};

static std::map<std::string, NodePhysics> s_globalPhysics;
static bool s_isDragMode = false;
static bool s_isResetting = false;
static bool s_triggerBounce = false;
static float s_bounceStrength = 1500.f;
static std::string s_lastInputString = "";

class PhysicsMenuPopup : public FLAlertLayer {
protected:
    CCTextInputNode* m_bounceInput = nullptr;
    CCMenuItemToggler* m_dragToggler = nullptr;

public:
    static PhysicsMenuPopup* create() {
        auto ret = new PhysicsMenuPopup();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() {
        if (!FLAlertLayer::init(150)) return false;
        
        auto winSize = CCDirector::get()->getWinSize();
        
        auto bg = CCScale9Sprite::create("GJ_square01.png");
        bg->setContentSize({350.f, 250.f});
        bg->setPosition(winSize.width / 2, winSize.height / 2);
        this->m_mainLayer->addChild(bg);
        
        auto menu = CCMenu::create();
        menu->setPosition(0, 0);
        menu->setTouchPriority(-501); 
        this->m_mainLayer->addChild(menu);
        
        auto closeSpr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        auto closeBtn = CCMenuItemSpriteExtra::create(closeSpr, this, menu_selector(PhysicsMenuPopup::onClose));
        closeBtn->setPosition(winSize.width / 2 - 175.f, winSize.height / 2 + 125.f);
        menu->addChild(closeBtn);
        
        auto checkOn = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
        auto checkOff = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
        checkOn->setScale(1.2f);
        checkOff->setScale(1.2f);
        m_dragToggler = CCMenuItemToggler::create(checkOff, checkOn, this, menu_selector(PhysicsMenuPopup::onToggleDrag));
        m_dragToggler->setPosition(winSize.width / 2 - 100.f, winSize.height / 2 + 50.f);
        m_dragToggler->toggle(s_isDragMode);
        menu->addChild(m_dragToggler);
        
        auto dragLabel = CCLabelBMFont::create("Enable Drag", "bigFont.fnt");
        dragLabel->setScale(0.7f);
        dragLabel->setAnchorPoint({0.f, 0.5f});
        dragLabel->setPosition(winSize.width / 2 - 60.f, winSize.height / 2 + 50.f);
        this->m_mainLayer->addChild(dragLabel);
        
        float boxX = winSize.width / 2 - 60.f; 
        float boxY = winSize.height / 2 - 10.f;

        auto inputBg = CCScale9Sprite::create("square02_small.png");
        inputBg->setContentSize({65.f, 30.f});
        inputBg->setOpacity(100);
        inputBg->setPosition(boxX, boxY);
        this->m_mainLayer->addChild(inputBg);
        
        m_bounceInput = CCTextInputNode::create(55.f, 30.f, "Bounce Force", "bigFont.fnt");
        m_bounceInput->setAllowedChars("0123456789.-");
        m_bounceInput->setAnchorPoint({0.f, 0.5f});
        m_bounceInput->setPosition(224.5, 151);
        m_bounceInput->setMaxLabelScale(0.6f);
        m_bounceInput->setLabelPlaceholderColor({150, 150, 150});
        m_bounceInput->setString(s_lastInputString.c_str());
        this->m_mainLayer->addChild(m_bounceInput);
        
        auto bounceSpr = ButtonSprite::create("Bounce!");
        bounceSpr->setScale(0.85f);
        auto bounceBtn = CCMenuItemSpriteExtra::create(bounceSpr, this, menu_selector(PhysicsMenuPopup::onBounce));
        bounceBtn->setPosition(boxX + 100.f, boxY); 
        menu->addChild(bounceBtn);
        
        auto resetSpr = ButtonSprite::create("Reset Physics");
        resetSpr->setScale(1.0f);
        auto resetBtn = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(PhysicsMenuPopup::onReset));
        resetBtn->setPosition(winSize.width / 2, winSize.height / 2 - 75.f);
        menu->addChild(resetBtn);

        auto infoSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        infoSpr->setScale(1.1f);
        auto infoBtn = CCMenuItemSpriteExtra::create(infoSpr, this, menu_selector(PhysicsMenuPopup::onInfo));
        infoBtn->setPosition(winSize.width / 2 + 150.f, winSize.height / 2 + 100.f);
        menu->addChild(infoBtn);
        
        this->setKeypadEnabled(true);
        this->setTouchEnabled(true);
        return true;
    }

    void registerWithTouchDispatcher() override {
        CCTouchDispatcher::get()->addTargetedDelegate(this, -500, true);
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override {
        auto winSize = CCDirector::get()->getWinSize();
        CCPoint localPos = this->m_mainLayer->convertToNodeSpace(touch->getLocation());
        CCRect inputRect = CCRectMake(
            winSize.width / 2 - 60.f - 32.5f, 
            winSize.height / 2 - 10.f - 15.f, 
            65.f, 30.f
        );
        if (inputRect.containsPoint(localPos)) {
            if (m_bounceInput) m_bounceInput->onClickTrackNode(true);
        } else {
            if (m_bounceInput) m_bounceInput->onClickTrackNode(false);
        }
        return true; 
    }

    void keyBackClicked() override {
        this->onClose(nullptr);
    }
    
    void onClose(CCObject*) {
        if (m_bounceInput) s_lastInputString = m_bounceInput->getString();
        this->removeFromParent();
    }
    
    void onToggleDrag(CCObject* sender) {
        s_isDragMode = !s_isDragMode;
    }
    
    void onBounce(CCObject*) {
        if (m_bounceInput) {
            std::string val = m_bounceInput->getString();
            s_lastInputString = val;
            
            if (val.empty()) {
                FLAlertLayer::create("Error", "Please enter a value!", "OK")->show();
                return;
            }

            auto parsedVal = geode::utils::numFromString<float>(val);
            if (parsedVal.isErr()) {
                FLAlertLayer::create("Error", "Invalid number format!", "OK")->show();
                return;
            }
            s_bounceStrength = parsedVal.unwrap();
        }
        s_triggerBounce = true;
    }

    void onReset(CCObject*) {
        s_isResetting = true;
        this->removeFromParent();
    }

    void onInfo(CCObject*) {
        FLAlertLayer::create("GD Physics", "<cg>Enable Drag</c>: Drag nodes on Mobile/Mac.\n<cb>Right Click</c>: Drag nodes on PC.\n<cy>Bounce</c>: Bounces the nodes.\n<cr>Reset</c>: Original positions.", "OK")->show();
    }
};

class PhysicsOverlay : public CCLayer {
protected:
    CCNode* m_draggedNode = nullptr;
    CCPoint m_lastTouchPos;
    CCPoint m_prevNodePos;
    bool m_isRightClickDragging = false; 
    CCMenu* m_uiMenu = nullptr;

public:
    static PhysicsOverlay* create() {
        auto ret = new PhysicsOverlay();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) return false;
        auto winSize = CCDirector::get()->getWinSize();

        m_uiMenu = CCMenu::create();
        m_uiMenu->setPosition(winSize.width - 30.f, winSize.height - 30.f);
        m_uiMenu->setTouchPriority(-501);
        
        auto gearSpr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        gearSpr->setScale(0.8f);
        auto menuBtn = CCMenuItemSpriteExtra::create(gearSpr, this, menu_selector(PhysicsOverlay::onOpenMenu));
        m_uiMenu->addChild(menuBtn);
        
        this->addChild(m_uiMenu, 100);

        this->setTouchEnabled(true);
        this->schedule(schedule_selector(PhysicsOverlay::physicsTick));

        return true;
    }

    void registerWithTouchDispatcher() override {
        CCTouchDispatcher::get()->addTargetedDelegate(this, -500, true);
    }

    void onOpenMenu(CCObject*) {
        auto popup = PhysicsMenuPopup::create();
        popup->show();
    }

    void findButtons(CCNode* node, std::vector<CCNode*>& buttons) {
        if (!node || node == this || node == m_uiMenu) return;

        if (typeinfo_cast<GJDropDownLayer*>(node) || 
            typeinfo_cast<OptionsLayer*>(node) ||
            typeinfo_cast<FLAlertLayer*>(node)) {
            return;
        }

        std::string id = node->getID();
        if (id == "close-button" || id == "back-button" || id == "exit-button" || id == "options-button" || id == "main-title") return; 

        bool isTarget = typeinfo_cast<CCMenuItem*>(node) || id == "creator-title";

        if (isTarget && !id.empty()) {
            buttons.push_back(node);
        } else {
            for (auto child : node->getChildrenExt<CCNode*>()) findButtons(child, buttons);
        }
    }

    std::vector<CCNode*> getPhysicsNodes() {
        std::vector<CCNode*> nodes;
        findButtons(this->getParent(), nodes);
        return nodes;
    }

    void forceFirstFrame() {
        auto nodes = getPhysicsNodes();
        bool hasNewNodes = false;

        for (auto node : nodes) {
            std::string id = node->getID();
            if (!s_globalPhysics[id].initialized) {
                node->stopAllActions(); 
                s_globalPhysics[id].originalPos = node->getPosition();
                s_globalPhysics[id].pos = s_globalPhysics[id].originalPos + ccp(0, 300.f);
                s_globalPhysics[id].velocity = ccp(0, 0);
                s_globalPhysics[id].initialized = true;
                hasNewNodes = true;
            }
        }

        if (hasNewNodes) {
            for(int i = 0; i < 300; i++) simulatePhysics(1.0f/60.0f, nodes);
            for (auto node : nodes) node->setPosition(s_globalPhysics[node->getID()].pos);
        }
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override {
        if (m_uiMenu && m_uiMenu->isVisible()) {
            for (auto child : m_uiMenu->getChildrenExt<CCNode*>()) {
                CCPoint localPos = child->convertToNodeSpace(touch->getLocation());
                CCSize size = child->getContentSize();
                if (CCRectMake(0, 0, size.width, size.height).containsPoint(localPos)) return false;
            }
        }

        if (!s_isDragMode) return false;

        auto touchPos = touch->getLocation();
        auto nodes = getPhysicsNodes();
        
        for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
            auto node = *it;
            std::string id = node->getID();
            if (!s_globalPhysics[id].initialized) continue;

            CCPoint parentSpaceTouch = node->getParent()->convertToNodeSpace(touchPos);
            if (node->boundingBox().containsPoint(parentSpaceTouch)) {
                m_draggedNode = node;
                m_lastTouchPos = touchPos;
                s_globalPhysics[id].velocity = ccp(0, 0); 
                m_prevNodePos = s_globalPhysics[id].pos;
                return true; 
            }
        }
        return false; 
    }

    void ccTouchMoved(CCTouch* touch, CCEvent* event) override {
        if (m_draggedNode && !m_isRightClickDragging) {
            auto touchPos = touch->getLocation();
            auto localPos = m_draggedNode->getParent()->convertToNodeSpace(touchPos);
            s_globalPhysics[m_draggedNode->getID()].pos = localPos;
            m_lastTouchPos = touchPos;
        }
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (m_draggedNode && !m_isRightClickDragging) m_draggedNode = nullptr;
    }

    void ccTouchCancelled(CCTouch* touch, CCEvent* event) override { ccTouchEnded(touch, event); }

    void simulatePhysics(float dt, const std::vector<CCNode*>& nodes) {
        float gravity = static_cast<float>(Mod::get()->getSettingValue<double>("gravity"));
        float wallBounce = static_cast<float>(Mod::get()->getSettingValue<double>("wall-bounce"));
        auto winSize = CCDirector::get()->getWinSize();

        for (auto node : nodes) {
            if (node == m_draggedNode) continue;
            std::string id = node->getID();
            auto& phys = s_globalPhysics[id];
            auto size = node->getScaledContentSize();
            float r = std::max(15.f, (size.width + size.height) / 4.0f);

            phys.velocity.y += gravity * dt;
            auto newPos = phys.pos + phys.velocity * dt;
            auto worldPos = node->getParent()->convertToWorldSpace(newPos);

            if (worldPos.y - r < 0.f) {
                worldPos.y = r;
                phys.velocity.y *= wallBounce;
                phys.velocity.x *= 0.95f; 
            } else if (worldPos.y + r > winSize.height) {
                worldPos.y = winSize.height - r;
                phys.velocity.y *= wallBounce;
            }
            if (worldPos.x - r < 0.f) {
                worldPos.x = r;
                phys.velocity.x *= wallBounce;
            } else if (worldPos.x + r > winSize.width) {
                worldPos.x = winSize.width - r;
                phys.velocity.x *= wallBounce;
            }

            phys.pos = node->getParent()->convertToNodeSpace(worldPos);
        }

        for (size_t i = 0; i < nodes.size(); i++) {
            for (size_t j = i + 1; j < nodes.size(); j++) {
                auto nodeA = nodes[i];
                auto nodeB = nodes[j];
                std::string idA = nodeA->getID();
                std::string idB = nodeB->getID();

                auto sizeA = nodeA->getScaledContentSize();
                auto sizeB = nodeB->getScaledContentSize();
                float rA = std::max(15.f, (sizeA.width + sizeA.height) / 4.0f);
                float rB = std::max(15.f, (sizeB.width + sizeB.height) / 4.0f);

                auto posA = nodeA->getParent()->convertToWorldSpace(s_globalPhysics[idA].pos);
                auto posB = nodeB->getParent()->convertToWorldSpace(s_globalPhysics[idB].pos);

                float dx = posB.x - posA.x;
                float dy = posB.y - posA.y;
                float dist = std::sqrt(dx*dx + dy*dy);
                float minDist = rA + rB;

                if (dist < minDist) {
                    if (dist < 0.001f) {
                        dx = (rand() % 100 - 50) / 100.f;
                        dy = (rand() % 100 - 50) / 100.f;
                        dist = 1.0f;
                    }
                    float overlap = minDist - dist;
                    float nx = dx / dist; 
                    float ny = dy / dist; 

                    if (nodeA == m_draggedNode) { posB.x += nx * overlap; posB.y += ny * overlap; }
                    else if (nodeB == m_draggedNode) { posA.x -= nx * overlap; posA.y -= ny * overlap; }
                    else {
                        posA.x -= nx * overlap * 0.5f; posA.y -= ny * overlap * 0.5f;
                        posB.x += nx * overlap * 0.5f; posB.y += ny * overlap * 0.5f;
                    }

                    s_globalPhysics[idA].pos = nodeA->getParent()->convertToNodeSpace(posA);
                    s_globalPhysics[idB].pos = nodeB->getParent()->convertToNodeSpace(posB);

                    if (nodeA != m_draggedNode && nodeB != m_draggedNode) {
                        float rvx = s_globalPhysics[idB].velocity.x - s_globalPhysics[idA].velocity.x;
                        float rvy = s_globalPhysics[idB].velocity.y - s_globalPhysics[idA].velocity.y;
                        float velAlongNormal = rvx * nx + rvy * ny;

                        if (velAlongNormal < 0) {
                            float bounciness = std::abs(wallBounce) + 0.2f; 
                            float impulse = -(1.0f + bounciness) * velAlongNormal;
                            impulse /= 2.0f; 

                            s_globalPhysics[idA].velocity.x -= impulse * nx;
                            s_globalPhysics[idA].velocity.y -= impulse * ny;
                            s_globalPhysics[idB].velocity.x += impulse * nx;
                            s_globalPhysics[idB].velocity.y += impulse * ny;
                        }
                    }
                }
            }
        }
    }

    void physicsTick(float dt) {
        if (dt > 0.05f) dt = 0.05f;
        auto nodes = getPhysicsNodes();
        bool hasNewNodes = false;

        auto winSize = CCDirector::get()->getWinSize();
        CCPoint mousePos = ccp(0, 0);
        bool isRightClick = false;

#ifdef GEODE_IS_WINDOWS
        isRightClick = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        if (isRightClick) {
            POINT pt;
            if (GetCursorPos(&pt)) {
                HWND hwnd = WindowFromDC(wglGetCurrentDC());
                if (hwnd) {
                    ScreenToClient(hwnd, &pt);
                    auto frameSize = CCDirector::get()->getOpenGLView()->getFrameSize();
                    float mouseX = (pt.x / frameSize.width) * winSize.width;
                    float mouseY = winSize.height - ((pt.y / frameSize.height) * winSize.height);
                    mousePos = ccp(mouseX, mouseY);
                }
            }
        }
#endif

        if (isRightClick) {
            if (!m_draggedNode) {
                for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
                    auto node = *it;
                    std::string id = node->getID();
                    if (!s_globalPhysics[id].initialized) continue;

                    CCPoint parentSpaceTouch = node->getParent()->convertToNodeSpace(mousePos);
                    if (node->boundingBox().containsPoint(parentSpaceTouch)) {
                        m_draggedNode = node;
                        m_lastTouchPos = mousePos;
                        m_isRightClickDragging = true;
                        s_globalPhysics[id].velocity = ccp(0, 0);
                        m_prevNodePos = s_globalPhysics[id].pos;
                        break;
                    }
                }
            } else if (m_isRightClickDragging) {
                auto localPos = m_draggedNode->getParent()->convertToNodeSpace(mousePos);
                s_globalPhysics[m_draggedNode->getID()].pos = localPos;
                m_lastTouchPos = mousePos;
            }
        } else if (m_isRightClickDragging && m_draggedNode) {
            m_draggedNode = nullptr;
            m_isRightClickDragging = false;
        }

        if (m_draggedNode) {
            std::string id = m_draggedNode->getID();
            if (dt > 0.0001f) {
                CCPoint currentPos = s_globalPhysics[id].pos;
                s_globalPhysics[id].velocity = ccpMult(ccpSub(currentPos, m_prevNodePos), 1.0f / dt);
                m_prevNodePos = currentPos;
            }
        }

        if (s_isResetting) {
            for (auto node : nodes) {
                std::string id = node->getID();
                if (s_globalPhysics.count(id)) {
                    s_globalPhysics[id].pos = s_globalPhysics[id].originalPos;
                    s_globalPhysics[id].velocity = ccp(0, 0);
                }
            }
            s_isResetting = false;
        }

        if (s_triggerBounce) {
            for (auto node : nodes) {
                std::string id = node->getID();
                if (s_globalPhysics.count(id)) {
                    s_globalPhysics[id].velocity.y += s_bounceStrength;
                }
            }
            s_triggerBounce = false;
        }

        for (auto node : nodes) {
            std::string id = node->getID();
            if (!s_globalPhysics[id].initialized) {
                node->stopAllActions(); 
                s_globalPhysics[id].originalPos = node->getPosition();
                s_globalPhysics[id].pos = s_globalPhysics[id].originalPos + ccp(0, 300.f);
                s_globalPhysics[id].velocity = ccp(0, 0);
                s_globalPhysics[id].initialized = true;
                hasNewNodes = true;
            }
        }

        if (hasNewNodes) {
            for(int i = 0; i < 300; i++) simulatePhysics(1.0f/60.0f, nodes);
        } else {
            simulatePhysics(dt, nodes);
        }

        for (auto node : nodes) {
            node->setPosition(s_globalPhysics[node->getID()].pos);
        }
    }
};

PhysicsOverlay* addPhysicsOverlay(CCLayer* layer) {
    auto overlay = PhysicsOverlay::create();
    overlay->setID("physics-overlay"_spr);
    overlay->setZOrder(105); 
    layer->addChild(overlay);
    return overlay;
}

class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (Mod::get()->getSettingValue<bool>("menu-layer-physics")) {
            auto overlay = addPhysicsOverlay(this);
            overlay->forceFirstFrame();
        }
        return true;
    }
};

class $modify(MyCreatorLayer, CreatorLayer) {
    bool init() {
        if (!CreatorLayer::init()) return false;
        if (Mod::get()->getSettingValue<bool>("creator-layer-physics")) {
            auto overlay = addPhysicsOverlay(this);
            overlay->forceFirstFrame();
        }
        return true;
    }
};