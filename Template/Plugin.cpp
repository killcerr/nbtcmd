#include "pch.h"
#include "Version.h"
#include <EventAPI.h>
#include <LoggerAPI.h>
#include <MC/Level.hpp>
#include <MC/BlockInstance.hpp>
#include <MC/Block.hpp>
#include <MC/BlockSource.hpp>
#include <MC/Actor.hpp>
#include <MC/Player.hpp>
#include <MC/ItemStack.hpp>
#include <LLAPI.h>
#include <MC/OverworldGenerator.hpp>
#include <MC/Dimension.hpp>
#include <MC/NetherDimension.hpp>
#include <MC/OverworldDimension.hpp>
#include <MC/TheEndDimension.hpp>
#include <MC/NetherGenerator.hpp>
#include <MC/OverworldGenerator.hpp>
#include <MC/TheEndGenerator.hpp>
#include <MC/WorldGenerator.hpp>
#include <MC/VanillaDimensions.hpp>
#include <MC/VanillaFeatures.hpp>
#include <MC/Feature.hpp>
#include <MC/FeatureHelper.hpp>
#include <MC/FeatureRegistry.hpp>
#include <ctime>
#include <HookAPI.h>
#include <MC/Dimension.hpp>
#include <MC/BiomeRegistry.hpp>
#include <thread>
#include <MC/Mob.hpp>
#include <MC/Monster.hpp>
#include <MC/Player.hpp>
#include <MC/Village.hpp>
#include <ScheduleAPI.h>
#include <RegCommandAPI.h>
#include <MC/ITreeFeature.hpp>
#include <MC/CaveFeatureUtils.hpp>
#include <MC/CaveFeature.hpp>
#include <MC/BlockPosIterator.hpp>
#include <MC/ServerPlayer.hpp>
#include <RegCommandAPI.h>
#include <MC/BlockVolumeTarget.hpp>
#include <MC/Biome.hpp>

Logger logger(PLUGIN_NAME);

inline void CheckProtocolVersion() {
#ifdef TARGET_BDS_PROTOCOL_VERSION
    auto currentProtocol = LL::getServerProtocolVersion();
    if (TARGET_BDS_PROTOCOL_VERSION != currentProtocol)
    {
        logger.warn("Protocol version not match, target version: {}, current version: {}.",
            TARGET_BDS_PROTOCOL_VERSION, currentProtocol);
        logger.warn("This will most likely crash the server, please use the Plugin that matches the BDS version!");
    }
#endif // TARGET_BDS_PROTOCOL_VERSION
}

class NbtCommand : public Command
{
	bool modeIsSet;
	CommandSelector<Mob> selector; 
	bool mobIsSet;
	string nbt;
	bool nbtIsSet;
	BlockPos blockPos;
	bool blockPosIsSet;
	string structPath;
	bool structPathIsSet;
public :
	void execute(CommandOrigin const& ori, CommandOutput& output) const override
	{
		if (mobIsSet)
		{
			if (nbtIsSet)
			{
				auto mobs = selector.results(ori);
				for (auto mob : mobs)
				{
					CompoundTag* compoundTag = &*CompoundTag::fromSNBT(nbt);
					mob->setNbt(compoundTag);
				}
				output.success(std::to_string(mobs.count()) + " nbt:" + nbt);
				return;
			}
			else
			{
				auto mobs = selector.results(ori);
				for (auto mob : mobs)
				{
					output.success("type:" + mob->getTypeName() + " nametag:" + mob->getNameTag() + " nbt:" + mob->getNbt()->toSNBT());
				}
				return;
			}
		}
		else if (blockPosIsSet)
		{
			if (nbtIsSet)
			{
				if (ori.getPlayer()->isPlayer())
				{
					auto block = Level::getBlock(blockPos, ori.getPlayer()->getDimensionId());
					CompoundTag* compoundTag = &*CompoundTag::fromSNBT(nbt);
					block->setNbt(compoundTag);
					output.success("type:" + block->getTypeName() + " nbt:" + nbt);
					return;
				}
				else
				{
					output.error("not player");
				}
			}
			else
			{
				if (ori.getPlayer()->isPlayer())
				{
					auto block = Level::getBlock(blockPos, ori.getPlayer()->getDimensionId());
					output.success("type:" + block->getTypeName() + " nbt:" + block->getNbt()->toSNBT());
					return;
				}
				else
				{
					output.error("not player");
				}
			}
		}
		else if (structPathIsSet)
		{
			if (nbtIsSet)
			{

			}
			else
			{

			}
		}
		else
		{
			output.error("commands.stats.failed");
		}
	}
	static void setup(CommandRegistry* registry)
	{
		using namespace RegisterCommandHelper;
		registry->registerCommand("nbt", "nbt", CommandPermissionLevel::Admin, { (CommandFlagValue)0 }, { (CommandFlagValue)0x80 });
		registry->registerOverload<NbtCommand>("nbt", makeOptional(&NbtCommand::selector, "target:target", &NbtCommand::mobIsSet));
		registry->registerOverload<NbtCommand>("nbt", makeOptional(&NbtCommand::selector, "target:target", &NbtCommand::mobIsSet), makeOptional(&NbtCommand::nbt,"nbt:string",&NbtCommand::nbtIsSet));
		registry->registerOverload<NbtCommand>("nbt", makeOptional(&NbtCommand::blockPos, "blockPos:block", &NbtCommand::blockPosIsSet));
		registry->registerOverload<NbtCommand>("nbt", makeOptional(&NbtCommand::blockPos, "blockPos:block", &NbtCommand::blockPosIsSet), makeOptional(&NbtCommand::nbt, "nbt:string", &NbtCommand::nbtIsSet));
		registry->registerOverload<NbtCommand>("nbt", makeOptional(&NbtCommand::structPath, "string:structPath", &NbtCommand::structPathIsSet));
		registry->registerOverload<NbtCommand>("nbt", makeOptional(&NbtCommand::structPath, "string:structPath", &NbtCommand::structPathIsSet), makeOptional(&NbtCommand::nbt, "nbt:string", &NbtCommand::nbtIsSet));
	}
};

void PluginInit()
{
    CheckProtocolVersion();
    time_t date;
	time(&date);
	auto localDate = localtime(&date);
	logger.setFile("./logs/nbtcmd.logs/"+std::to_string(localDate->tm_year+ 1900)+"."+ std::to_string(localDate->tm_mon) + "."+ std::to_string(localDate->tm_wday) + " "+ std::to_string(localDate->tm_hour) + ":"+ std::to_string(localDate->tm_min) + ":"+std::to_string(localDate->tm_sec));
	logger.info("nbtcmd loaded");
    Event::RegCmdEvent::subscribe([](Event::RegCmdEvent regCmdEvent) {
		NbtCommand::setup(regCmdEvent.mCommandRegistry);
		return true;
		});
}
