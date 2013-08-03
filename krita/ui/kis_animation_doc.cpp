/*
 *  Copyright (c) 2013 Somsubhra Bairi <somsubhra.bairi@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License, or(at you option)
 *  any later version..
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include "kis_animation_doc.h"
#include "kis_animation_part.h"
#include "kis_animation.h"
#include <kis_paint_layer.h>
#include <kis_image.h>
#include <kis_group_layer.h>
#include <kranim/kis_kranim_saver.h>
#include <kranim/kis_kranim_loader.h>
#include <kranim/kis_kranim_frame_loader.h>
#include <kranim/kis_kranim_frame_saver.h>
#include <KoFilterManager.h>

#define APP_MIMETYPE "application/x-krita-animation"
static const char CURRENT_DTD_VERSION[] = "1.0";


class KisAnimationDoc::KisAnimationDocPrivate{
public:
    KisAnimationDocPrivate()
        :kranimSaver(0),
          kranimLoader(0)
    {
    }

    ~KisAnimationDocPrivate(){

    }

    QDomDocument doc;
    QDomElement root;
    KisKranimSaver* kranimSaver;
    KisKranimLoader* kranimLoader;
    KisKranimFrameLoader* frameLoader;
    KisKranimFrameSaver* frameSaver;
    KisLayer* newFrame;
    bool saved;
};

KisAnimationDoc::KisAnimationDoc()
    : KisDoc2(new KisAnimationPart),
      m_d_anim(new KisAnimationDocPrivate())
{
    setMimeType(APP_MIMETYPE);

    m_d_anim->kranimLoader = 0;
    m_d_anim->saved = false;
    m_d_anim->newFrame = 0;
}

KisAnimationDoc::~KisAnimationDoc(){

}

void KisAnimationDoc::frameSelectionChanged(QRect frame)
{
    kWarning() << frame;
    if(!m_d_anim->saved){
        kWarning() << this->documentPart()->url();
        m_d_anim->kranimSaver = new KisKranimSaver(this);
        this->preSaveAnimation();
    }
}

void KisAnimationDoc::addBlankFrame(QRect frame){

    //Save the previous frame over here
    this->documentPart()->save();

    KisAnimation* animation = dynamic_cast<KisAnimationPart*>(this->documentPart())->animation();

    KisImageWSP image = new KisImage(createUndoStore(), animation->width(), animation->height(), animation->colorSpace(), animation->name());
    connect(image.data(), SIGNAL(sigImageModified()), this, SLOT(setImageModified()));
    image->setResolution(animation->resolution(), animation->resolution());


    m_d_anim->newFrame = new KisPaintLayer(image.data(), image->nextLayerName(), animation->bgColor().opacityU8(), animation->colorSpace());
    m_d_anim->newFrame->setName("testFrame");
    m_d_anim->newFrame->paintDevice()->setDefaultPixel(animation->bgColor().data());
    image->addNode(m_d_anim->newFrame/*.data()*/, image->rootLayer().data());

    //Load all the layers here

    setCurrentImage(image);
}

void KisAnimationDoc::addKeyFrame(QRect frame){
    kWarning() << "Adding keyframe";
}

bool KisAnimationDoc::completeSaving(KoStore *store)
{
    QString uri = url().url();

    if(!m_d_anim->saved){
        m_d_anim->kranimSaver->saveBinaryData(store, this->image(), uri, isStoredExtern());
    }
    else{
        QDomElement e = m_d_anim->doc.createElement("frame");
        e.setAttribute("number", 1);
        m_d_anim->root.appendChild(e);
        m_d_anim->kranimSaver->saveFrame(store, m_d_anim->newFrame);
    }

    return true;
}

QDomDocument KisAnimationDoc::saveXML()
{
    m_d_anim->doc = createDomDocument("animation", CURRENT_DTD_VERSION);

    m_d_anim->root = m_d_anim->doc.documentElement();

    m_d_anim->root.setAttribute("editor","Krita Animation");

    m_d_anim->root.setAttribute("syntaxVersion", "1");

    m_d_anim->root.appendChild(m_d_anim->kranimSaver->saveMetaData(m_d_anim->doc));
    m_d_anim->root.appendChild(m_d_anim->kranimSaver->saveXML(m_d_anim->doc, this->image()));

    return m_d_anim->doc;
}

bool KisAnimationDoc::loadXML(const KoXmlDocument &doc, KoStore *store)
{
    KoXmlElement root;
    QString attr;
    KoXmlNode node;
    return true;
}

bool KisAnimationDoc::completeLoading(KoStore *store)
{
    m_d_anim->kranimLoader = new KisKranimLoader(this);
    return true;
}

void KisAnimationDoc::preSaveAnimation(){
    KisAnimation* animation = dynamic_cast<KisAnimationPart*>(this->documentPart())->animation();

    KUrl url = this->documentPart()->url();

    QByteArray nativeFormat = this->nativeFormatMimeType();
    QStringList mimeFilter = KoFilterManager::mimeFilter(nativeFormat, KoFilterManager::Export,
                                                         this->extraNativeMimeTypes(KoDocument::ForExport));
    kWarning() << nativeFormat;
    if(!mimeFilter.contains(nativeFormat)){
        kWarning() << "No output filter";
    }
    this->setOutputMimeType(nativeFormat, 0);

    KUrl newUrl(url.directory()+"/"+animation->name()+".kranim");

    m_d_anim->saved = this->documentPart()->saveAs(newUrl);
}

#include "kis_animation_doc.moc"
