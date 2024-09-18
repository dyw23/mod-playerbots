/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include <functional>

#include "SuggestWhatToDoAction.h"
#include "ServerFacade.h"
#include "ChannelMgr.h"
#include "Event.h"
#include "ItemVisitors.h"
#include "AiFactory.h"
#include "ChatHelper.h"
#include "Playerbots.h"
#include "PlayerbotTextMgr.h"
#include "Config.h"
#include "BroadcastHelper.h"
#include "AiFactory.h"
#include "ChannelMgr.h"
#include "ChatHelper.h"
#include "Config.h"
#include "Event.h"
#include "GuildMgr.h"
#include "ItemVisitors.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include "ServerFacade.h"

enum eTalkType
{
    /*0x18*/ General = ChannelFlags::CHANNEL_FLAG_GENERAL | ChannelFlags::CHANNEL_FLAG_NOT_LFG,
    /*0x3C*/ Trade = ChannelFlags::CHANNEL_FLAG_CITY | ChannelFlags::CHANNEL_FLAG_GENERAL |
                     ChannelFlags::CHANNEL_FLAG_NOT_LFG | ChannelFlags::CHANNEL_FLAG_TRADE,
    /*0x18*/ LocalDefence = ChannelFlags::CHANNEL_FLAG_GENERAL | ChannelFlags::CHANNEL_FLAG_NOT_LFG,
    /*x038*/ GuildRecruitment =
        ChannelFlags::CHANNEL_FLAG_CITY | ChannelFlags::CHANNEL_FLAG_GENERAL | ChannelFlags::CHANNEL_FLAG_NOT_LFG,
    /*0x50*/ LookingForGroup = ChannelFlags::CHANNEL_FLAG_LFG | ChannelFlags::CHANNEL_FLAG_GENERAL
};

std::map<std::string, uint8> SuggestDungeonAction::instances;
std::map<std::string, uint8> SuggestWhatToDoAction::factions;

SuggestWhatToDoAction::SuggestWhatToDoAction(PlayerbotAI* botAI, std::string const name)
    : InventoryAction{botAI, name}, _dbc_locale{sWorld->GetDefaultDbcLocale()}
{
    suggestions.push_back(std::bind(&SuggestWhatToDoAction::specificQuest, this));
    suggestions.push_back(std::bind(&SuggestWhatToDoAction::grindReputation, this));
    suggestions.push_back(std::bind(&SuggestWhatToDoAction::something, this));
    suggestions.push_back(std::bind(&SuggestWhatToDoAction::grindMaterials, this));
    suggestions.push_back(std::bind(&SuggestWhatToDoAction::somethingToxic, this));
    suggestions.push_back(std::bind(&SuggestWhatToDoAction::toxicLinks, this));
}

bool SuggestWhatToDoAction::isUseful()
{
    if (!sRandomPlayerbotMgr->IsRandomBot(bot) || bot->GetGroup() || bot->GetInstanceId() || bot->GetBattleground())
        return false;

    std::string qualifier = "suggest what to do";
    time_t lastSaid = AI_VALUE2(time_t, "last said", qualifier);
    return (time(0) - lastSaid) > 30;
}

bool SuggestWhatToDoAction::Execute(Event event)
{
    uint32 index = rand() % suggestions.size();
    auto fnct_ptr = suggestions[index];
    fnct_ptr();

    std::string const qualifier = "suggest what to do";
    time_t lastSaid = AI_VALUE2(time_t, "last said", qualifier);
    botAI->GetAiObjectContext()->GetValue<time_t>("last said", qualifier)->Set(time(nullptr) + urand(1, 60));

    return true;
}

std::vector<uint32> SuggestWhatToDoAction::GetIncompletedQuests()
{
    std::vector<uint32> result;

    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        QuestStatus status = bot->GetQuestStatus(questId);
        if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_NONE)
            result.push_back(questId);
    }

    return result;
}

void SuggestWhatToDoAction::specificQuest()
{
    std::vector<uint32> quests = GetIncompletedQuests();
    if (quests.empty())
        return;

    BroadcastHelper::BroadcastSuggestQuest(botAI, quests, bot);
}

void SuggestWhatToDoAction::grindMaterials()
{
    /*if (bot->GetLevel() <= 5)
        return;

    auto result = CharacterDatabase.Query("SELECT distinct category, multiplier FROM ahbot_category where category not
    in ('other', 'quest', 'trade', 'reagent') and multiplier > 3 order by multiplier desc limit 10"); if (!result)
        return;

    std::map<std::string, double> categories;
    do
    {
        Field* fields = result->Fetch();
        categories[fields[0].Get<std::string>()] = fields[1].Get<float>();
    } while (result->NextRow());

    for (std::map<std::string, double>::iterator i = categories.begin(); i != categories.end(); ++i)
    {
        if (urand(0, 10) < 3) {
            std::string name = i->first;
            double multiplier = i->second;

            for (int j = 0; j < ahbot::CategoryList::instance.size(); j++)
            {
                ahbot::Category* category = ahbot::CategoryList::instance[j];
                if (name == category->GetName())
                {
                    std::string item = category->GetLabel();
                    transform(item.begin(), item.end(), item.begin(), ::tolower);
                    std::ostringstream itemout;
                    itemout << "|c0000b000" << item << "|r";
                    item = itemout.str();

                    std::map<std::string, std::string> placeholders;
                    placeholders["%role"] = chat->formatClass(bot, AiFactory::GetPlayerSpecTab(bot));
                    placeholders["%category"] = item;

                    spam(BOT_TEXT2("suggest_trade", placeholders), urand(0, 1) ? 0x3C : 0x18, !urand(0, 2), !urand(0,
    3)); return;
                }
            }
        }
    }*/
}

void SuggestWhatToDoAction::grindReputation()
{
    if (factions.empty())
    {
        factions["银色黎明"] = 60;
        factions["血帆海盗"] = 40;
        factions["诺兹多姆的子嗣"] = 60;
        factions["塞纳里奥议会"] = 55;
        factions["暗月马戏团"] = 20;
        factions["海达希亚水元素"] = 60;
        factions["拉文霍德"] = 20;
        factions["瑟银兄弟会"] = 40;
        factions["木喉要塞"] = 50;
        factions["霜刃豹训练师"] = 50;
        factions["藏宝海湾"] = 30;
        factions["永望镇"] = 40;
        factions["加基森"] = 50;
        factions["棘齿城"] = 20;

        factions["灰舌死誓者"] = 70;
        factions["塞纳里奥远征队"] = 62;
        factions["星界财团"] = 65;
        factions["荣耀堡"] = 66;
        factions["时光守护者"] = 68;
        factions["虚空幼龙"] = 65;
        factions["奥格瑞拉"] = 65;
        factions["流沙之鳞"] = 65;
        factions["孢子村"] = 65;
        factions["幽暗城"] = 10;
        factions["紫罗兰之眼"] = 70;

        factions["银色十字军"] = 75;
        factions["灰烬审判军"] = 75;
        factions["卡鲁亚克"] = 72;
        factions["肯瑞托"] = 75;
        factions["黑锋骑士团"] = 77;
        factions["霍迪尔之子"] = 78;
        factions["龙眠联军"] = 77;
    }

    std::vector<std::string> levels;
    levels.push_back("尊敬");
    levels.push_back("崇敬");
    levels.push_back("崇拜");

    std::vector<std::string> allowedFactions;
    for (auto it : factions)
    {
        if ((int)bot->GetLevel() >= it.second) allowedFactions.push_back(it.first);
    }

    if (allowedFactions.empty()) return;

    BroadcastHelper::BroadcastSuggestGrindReputation(botAI, levels, allowedFactions, bot);
}

void SuggestWhatToDoAction::something()
{
    BroadcastHelper::BroadcastSuggestSomething(botAI, bot);
}

void SuggestWhatToDoAction::somethingToxic()
{
    BroadcastHelper::BroadcastSuggestSomethingToxic(botAI, bot);
}

void SuggestWhatToDoAction::toxicLinks()
{
    BroadcastHelper::BroadcastSuggestToxicLinks(botAI, bot);
}

void SuggestWhatToDoAction::thunderfury()
{
    BroadcastHelper::BroadcastSuggestThunderfury(botAI, bot);
}

class FindTradeItemsVisitor : public IterateItemsVisitor
{
public:
    FindTradeItemsVisitor(uint32 quality) : quality(quality), IterateItemsVisitor() {}

    bool Visit(Item* item) override
    {
        ItemTemplate const* proto = item->GetTemplate();
        if (proto->Quality != quality)
            return true;

        if (proto->Class == ITEM_CLASS_TRADE_GOODS && proto->Bonding == NO_BIND)
        {
            if (proto->Quality == ITEM_QUALITY_NORMAL && item->GetCount() > 1 &&
                item->GetCount() == item->GetMaxStackCount())
                stacks.push_back(proto->ItemId);

            items.push_back(proto->ItemId);
            count[proto->ItemId] += item->GetCount();
        }

        return true;
    }

    std::map<uint32, uint32> count;
    std::vector<uint32> stacks;
    std::vector<uint32> items;

private:
    uint32 quality;
};

SuggestDungeonAction::SuggestDungeonAction(PlayerbotAI* botAI) : SuggestWhatToDoAction(botAI, "suggest dungeon") {}

bool SuggestDungeonAction::Execute(Event event)
{
    // TODO: use sPlayerbotDungeonSuggestionMgr

    if (!sPlayerbotAIConfig->randomBotSuggestDungeons || bot->GetGroup())
        return false;

    if (instances.empty())
    {
        instances["怒焰裂谷"] = 15;
        instances["死亡矿井"] = 18;
        instances["哀嚎洞穴"] = 18;
        instances["影牙城堡"] = 25;
        instances["黑暗深渊"] = 20;
        instances["监狱"] = 20;
        instances["诺莫瑞根"] = 35;
        instances["剃刀沼泽"] = 35;
        instances["玛拉顿"] = 50;
        instances["血色修道院"] = 40;
        instances["奥达曼"] = 45;
        instances["厄运之槌"] = 58;
        instances["通灵学院"] = 59;
        instances["剃刀高地"] = 40;
        instances["斯坦索姆"] = 59;
        instances["祖尔法拉克"] = 45;
        instances["黑石深渊"] = 55;
        instances["阿塔哈卡神庙"] = 55;
        instances["黑石塔下层"] = 57;

        instances["地狱火堡垒"] = 65;
        instances["盘牙水库"] = 65;
        instances["奥金顿"] = 65;
        instances["时光之穴"] = 68;
        instances["风暴要塞"] = 69;
        instances["魔导师平台"] = 70;

        instances["乌特加德城堡"] = 75;
        instances["纳克萨玛斯"] = 75;
        instances["安卡赫特：古代王国"] = 75;
        instances["艾卓-尼鲁布"] = 75;
        instances["达克萨隆要塞"] = 75;
        instances["紫罗兰监狱"] = 80;
        instances["古达克"] = 77;
        instances["岩石大厅"] = 77;
        instances["闪电大厅"] = 77;
        instances["魔环"] = 77;
        instances["乌特加德之巅"] = 77;
        instances["冠军的试炼"] = 80;
        instances["灵魂熔炉"] = 80;
        instances["萨隆矿坑"] = 80;
        instances["映像大厅"] = 80;
    }

    std::vector<std::string> allowedInstances;
    for (auto it : instances)
    {
        if (bot->GetLevel() >= it.second) allowedInstances.push_back(it.first);
    }

    if (allowedInstances.empty()) return false;

    BroadcastHelper::BroadcastSuggestInstance(botAI, allowedInstances, bot);
    return true;
}

SuggestTradeAction::SuggestTradeAction(PlayerbotAI* botAI) : SuggestWhatToDoAction(botAI, "suggest trade") {}

bool SuggestTradeAction::Execute(Event event)
{
    uint32 quality = urand(0, 100);
    if (quality > 95)
        quality = ITEM_QUALITY_LEGENDARY;
    else if (quality > 90)
        quality = ITEM_QUALITY_EPIC;
    else if (quality > 75)
        quality = ITEM_QUALITY_RARE;
    else if (quality > 50)
        quality = ITEM_QUALITY_UNCOMMON;
    else
        quality = ITEM_QUALITY_NORMAL;

    uint32 item = 0, count = 0;
    while (quality-- > ITEM_QUALITY_POOR)
    {
        FindTradeItemsVisitor visitor(quality);
        IterateItems(&visitor);
        if (!visitor.stacks.empty())
        {
            uint32 index = urand(0, visitor.stacks.size() - 1);
            item = visitor.stacks[index];
        }

        if (!item)
        {
            if (!visitor.items.empty())
            {
                uint32 index = urand(0, visitor.items.size() - 1);
                item = visitor.items[index];
            }
        }

        if (item)
        {
            count = visitor.count[item];
            break;
        }
    }

    if (!item || !count)
        return false;

    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(item);
    if (!proto)
        return false;

    uint32 price = proto->SellPrice * sRandomPlayerbotMgr->GetSellMultiplier(bot) * count;
    if (!price)
        return false;

    BroadcastHelper::BroadcastSuggestSell(botAI, proto, count, price, bot);
    return true;
}
