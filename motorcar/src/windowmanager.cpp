#include "windowmanager.h"

using namespace motorcar;
WindowManager::WindowManager(Scene *scene, Seat *defaultSeat)
    :m_numSurfacesMapped(0)
    ,m_scene(scene)
    ,m_defaultSeat(defaultSeat)
{
}

WaylandSurfaceNode *WindowManager::createSurface(WaylandSurface *surface)
{
    WaylandSurfaceNode *surfaceNode = new WaylandSurfaceNode(surface, this->scene());
    m_surfaceMap.insert(std::pair<WaylandSurface *, motorcar::WaylandSurfaceNode *>(surface, surfaceNode));
    //ensureKeyboardFocusIsValid(surface);
    return surfaceNode;
}

void WindowManager::destroySurface(WaylandSurface *surface)
{
    WaylandSurfaceNode *surfaceNode = this->getSurfaceNode(surface);
    if(surfaceNode != NULL){

//        std::vector<motorcar::SceneGraphNode *> subtreeNodes = surfaceNode->nodesInSubtree();
//        for(motorcar::SceneGraphNode *node : subtreeNodes){
//            motorcar::WaylandSurfaceNode *subtreeSurfaceNode = dynamic_cast<motorcar::WaylandSurfaceNode *>(node);
//            if(subtreeSurfaceNode != NULL){
//                WaylandSurface *subtreeSurface = subtreeSurfaceNode->surface();

//                    std::map<WaylandSurface *, motorcar::WaylandSurfaceNode *>::iterator it = m_surfaceMap.find(subtreeSurface);
//                    if (it != m_surfaceMap.end()){
//                        std::cout << "nulling surfaceNode pointer: " << it->second  << " in surface map" <<std::endl;
//                        it->second = NULL;
//                    }

//            }
//        }
        m_surfaceMap.erase (surface);

        //std::cout << "attempting to delete surfaceNode pointer " << surfaceNode <<std::endl;

        delete surfaceNode;

        ensureKeyboardFocusIsValid(surface);


    }
}


WaylandSurfaceNode *WindowManager::mapSurface(motorcar::WaylandSurface *surface, WaylandSurface::SurfaceType surfaceType)
{
    glm::mat4 transform;
    SceneGraphNode *parentNode;

    int type = static_cast<int>(surfaceType);
    float popupZOffset = 0.05;


    if(type == WaylandSurface::SurfaceType::TOPLEVEL){
        parentNode = this->scene();
        transform = glm::mat4(1)
                      //  * glm::rotate(glm::mat4(1), -90.f, glm::vec3(0, 1, 0))
                        * glm::translate(glm::mat4(1), glm::vec3(0, 0 ,1.25f))
                       // * glm::rotate(glm::mat4(1), (-1 +  m_numSurfacesMapped % 3) * 30.f, glm::vec3(0, -1, 0))
                   // * glm::rotate(glm::mat4(1),  (-1 + m_numSurfacesMapped / 3) * 30.f, glm::vec3(-1, 0, 0))
                    * glm::translate(glm::mat4(1), glm::vec3(0,0.0,-1.5f))
                    * glm::mat4(1);
        m_numSurfacesMapped ++;
    }else if(type == WaylandSurface::SurfaceType::POPUP ||
             type == WaylandSurface::SurfaceType::TRANSIENT){

        WaylandSurfaceNode *parentSurfaceNode;
        //if(surface->parentSurface() != NULL){
        //   parentSurfaceNode = this->getSurfaceNode(surface->parentSurface());
        //    glm::vec2 localPos = glm::vec2(surface->position());
        if(defaultSeat()->pointerFocus() != NULL){
           parentSurfaceNode = this->getSurfaceNode(defaultSeat()->pointerFocus());
           glm::vec2 localPos = this->defaultSeat()->pointer()->localPositon();
           glm::vec3 position = glm::vec3(parentSurfaceNode->surfaceTransform() *
                                          glm::vec4((localPos + glm::vec2(surface->size()) / 2.0f) /
                                                       glm::vec2(parentSurfaceNode->surface()->size()), popupZOffset, 1));
           std::cout << "creating popup/transient window with parent " << parentSurfaceNode << " at position:" << std::endl;
           motorcar::Geometry::printVector(position);
           transform = glm::translate(glm::mat4(), position);
           parentNode = parentSurfaceNode;
        }else{
            std::cout << "WARNING: creating popup/transient window with no parent " << std::endl;
             parentNode = this->scene();
             transform = glm::mat4();
        }





    }else{
        transform = glm::mat4();
        parentNode = this->scene();
    }

    //WaylandSurfaceNode *surfaceNode = new WaylandSurfaceNode(surface, parentNode, transform);

    WaylandSurfaceNode *surfaceNode = getSurfaceNode(surface);

    if(surfaceNode != NULL){
        surfaceNode->surface()->setType(surfaceType);
        surfaceNode->setParentNode(parentNode);
        surfaceNode->setTransform(transform);
    }else{
        std::cout << "Warning: surface mapped before surfaceNode created, creating now" << std::endl;
        surfaceNode = new WaylandSurfaceNode(surface, parentNode, transform);
        m_surfaceMap.insert(std::pair<WaylandSurface *, motorcar::WaylandSurfaceNode *>(surface, surfaceNode));
    }

    if(type == WaylandSurface::SurfaceType::POPUP || type == WaylandSurface::SurfaceType::TOPLEVEL){
        this->defaultSeat()->setPointerFocus(surfaceNode->surface(), glm::vec2());
    }

    surfaceNode->setMapped(true);
    //ensureKeyboardFocusIsValid(surface);
    return surfaceNode;
}

void WindowManager::unmapSurface(WaylandSurface *surface)
{
    WaylandSurfaceNode *surfaceNode = this->getSurfaceNode(surface);
    if(surfaceNode != NULL){
        surfaceNode->setMapped(false);
        ensureKeyboardFocusIsValid(surface);
    }else{
        std::cout << "Warning: surface unmapped but doesnt have associated surfaceNode" <<std::endl;
    }
}

void WindowManager::sendEvent(const Event &event)
{
    WaylandSurface *focus;
    switch(event.type()){
    case Event::EventType::MOUSE:
        focus = event.seat()->pointerFocus();
        if(focus != NULL){
            focus->sendEvent(event);
        }
        break;
    case Event::EventType::KEYBOARD:
        focus = event.seat()->keyboardFocus();
        if(focus != NULL){
            focus->sendEvent(event);
        }
        break;
    default:
        break;
    }
}

WaylandSurfaceNode *WindowManager::getSurfaceNode(WaylandSurface *surface) const
{
    if(surface != NULL && m_surfaceMap.count(surface)){
        return m_surfaceMap.find(surface)->second;
    }else{
        return NULL;
    }

}

Scene *WindowManager::scene() const
{
    return m_scene;
}

void WindowManager::setScene(Scene *scene)
{
    m_scene = scene;
}
Seat *WindowManager::defaultSeat() const
{
    return m_defaultSeat;
}

void WindowManager::setDefaultSeat(Seat *defaultSeat)
{
    m_defaultSeat = defaultSeat;
}

void WindowManager::ensureKeyboardFocusIsValid(WaylandSurface *oldSurface)
{
    WaylandSurface *nextSurface = NULL;
    if(!m_surfaceMap.empty()){
        nextSurface = m_surfaceMap.begin()->first;
    }
    m_defaultSeat->ensureKeyboardFocusIsValid(oldSurface, nextSurface);
}








