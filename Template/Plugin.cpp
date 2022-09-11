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
#include <MC/Dimension.hpp>
#include <ctime>
#include <HookAPI.h>
#include <MC/Dimension.hpp>
#include <MC/Mob.hpp>
#include <MC/Player.hpp>
#include <ScheduleAPI.h>
#include <RegCommandAPI.h>
#include <MC/ServerPlayer.hpp>
#include <RegCommandAPI.h>
#include <MC/Packet.hpp>
#include <MC/SetActorDataPacket.hpp>
#include <MC/BlockActorDataPacket.hpp>
#include <MC/CommandOrigin.hpp>
#include <MC/Container.hpp>
#include <future>
#include <stdexcept>

Logger logger("nbt cmd");

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

auto asyncFileOut(const string fileName, const string content, const int prot)
{
	(void)std::async(std::launch::async, [](string fileName, string content, int prot) {
		std::ofstream fout;
		fout.open(fileName, prot);
		if (fout.is_open())
		{
			fout.write(content.c_str(), content.length());
		}
		else
		{
			throw  std::runtime_error("open error");
		}
		}, fileName, content, prot);
}
auto asyncFileIn(const string path)
{
	auto result = std::async(std::launch::async, [](const string path) {
		std::ifstream fin;
		fin.open(path, std::ios_base::in);
		string str;
		if (fin.is_open())
		{
			string temp;
			while (fin >> temp)
			{
				str += temp;
			}
			return str;
		}
		else
		{
			throw  std::runtime_error("open error");
		}
		}, path);
	return result;
}
class NbtCommand : public Command
{
	CommandSelector<Actor> selector; 
	bool actorIsSet;
	string nbt;
	bool nbtIsSet;
	CommandPosition blockPos;
	bool blockPosIsSet;
	enum TargetType :int
	{
		block = 0,
		actor = 1,
		item = 2
	}targetType;
	bool targetTypeIsSet;
public :
	void execute(CommandOrigin const& ori, CommandOutput& output) const override
	{
		if ((ori.getPlayer()->isPlayer() && ori.getPlayer()->isOP()) || !ori.getPlayer()->isPlayer())
		{
			if (targetTypeIsSet)
			{
				switch (targetType)
				{
				case NbtCommand::block:
					{
						if (blockPosIsSet)
						{
							auto block = ori.getPlayer()->isPlayer() ? ori.getPlayer()->getLevel().getBlock(blockPos.getBlockPos(Vec3(), Vec3()), ori.getPlayer()->getDimensionId()) : Global<Level>->getBlock(blockPos.getBlockPos(Vec3(), Vec3()), 0);
							if (nbtIsSet)
							{
								block->setNbt(CompoundTag::fromSNBT(nbt).get());
								for (auto player : Global<Level>->getAllPlayers())
								{
									player->sendUpdateBlockPacket(blockPos.getBlockPos(ori, Vec3()), *block);
								}
							}
							else
							{
								output.success(block->getNbt()->toSNBT());
							}
						}
						else
						{
							output.error("commands.stats.failed");
						}
						break;
					}
				case NbtCommand::actor:
					{
						if (actorIsSet)
						{
							if (nbtIsSet)
							{
								int count = 0;
								for (auto actor : selector.results(ori))
								{
									actor->setNbt(CompoundTag::fromSNBT(nbt).get());
									count++;
								}
								output.success("count:" + std::to_string(count));
							}
							else
							{
								int count = 0;
								for (auto actor : selector.results(ori))
								{
									output.addMessage(actor->getNbt()->toSNBT());
									count++;
								}
								output.success("count:" + std::to_string(count));
							}
						}
						else
						{
							output.error("commands.stats.failed");
						}
						break;
					}
				case NbtCommand::item:
					{
						if (actorIsSet)
						{
							if (nbtIsSet)
							{
								int count = 0;
								for (auto actor : selector.results(ori))
								{
									auto itemStack = ItemStack();
									itemStack.setNbt(CompoundTag::fromSNBT(nbt).get());
									actor->add(itemStack);
									count++;
								}

							}
							else
							{
								output.error("Can't read item nbt");
							}
						}
						else
						{
							output.error("commands.stats.failed");
						}
						break;
					}
				default:
					{
						output.error("commands.stats.failed");
						break;
					}
				}
			}
			else
			{
				output.error("commands.stats.failed");
			}
		}
	}
	static void setup(CommandRegistry* registry)
	{
		using namespace RegisterCommandHelper;
		registry->registerCommand("nbt", "nbt", CommandPermissionLevel::Any, { (CommandFlagValue)0 }, { (CommandFlagValue)0x80 });
		//registry->registerOverload<NbtCommand>("nbt", makeOptional(&NbtCommand::selector, "target", &NbtCommand::actorIsSet), makeOptional(&NbtCommand::nbt,"snbt",&NbtCommand::nbtIsSet));
		//registry->registerOverload<NbtCommand>("nbt", makeOptional(&NbtCommand::blockPos, "block", &NbtCommand::blockPosIsSet), makeOptional(&NbtCommand::nbt, "snbt", &NbtCommand::nbtIsSet));
		registry->addEnum<NbtCommand::TargetType>("targettype", { {"actor",NbtCommand::TargetType::actor},{"block",NbtCommand::TargetType::block},{"item",NbtCommand::TargetType::item} });
		registry->registerOverload<NbtCommand>("nbt", makeMandatory<NbtCommand, NbtCommand::TargetType>(&NbtCommand::targetType, "targettype", &NbtCommand::targetTypeIsSet), makeOptional(&NbtCommand::selector, "target", &NbtCommand::actorIsSet), makeOptional(&NbtCommand::nbt, "snbt", &NbtCommand::nbtIsSet));
		registry->registerOverload<NbtCommand>("nbt", makeMandatory<NbtCommand, NbtCommand::TargetType>(&NbtCommand::targetType, "targettype", &NbtCommand::targetTypeIsSet), makeOptional(&NbtCommand::blockPos, "block", &NbtCommand::blockPosIsSet), makeOptional(&NbtCommand::nbt, "snbt", &NbtCommand::nbtIsSet));
	}
};
//using json = basic_json<>;
class NewNbtCmd :public Command
{
	CommandSelector<Actor> fromActor;
	CommandSelector<Actor> toActor;
	CommandPosition fromBlock;
	CommandPosition toBlock;
	string fromPath;
	string toPath;
	int fromSolt;
	CommandSelector<Player> fromItem;
	int toSolt;
	CommandSelector<Player> toItem;
	//json fromJson;
	bool modeIsSet, fromActorIsSet, toActorIsSet, fromBlockIsSet, toBlockIsSet, fromPathIsSet, toPathIsSet, fromJsonIsSet, fromItemIsSet, fromSoltIsSet, toItemIsSet, toSoltIsSet;
	static string from(string path)
	{
		auto file = asyncFileIn(path);
		return file.get();
	}
	static string from(BlockPos pos,int dim)
	{
		return Global<Level>->getBlock(pos, dim)->getNbt()->toSNBT();
	}
	static string from(std::vector<Actor*> actors)
	{
		string result;
		int count = 0;
		for (auto actor : actors)
		{
			result += fmt::to_string(count);
			result += ":";
			result += actor->getNbt()->toSNBT();
			result += "\n";
			count++;
		}
		return result;
	}
	/*static string from(json snbt)
	{
		return string(snbt);
	
	}*/
	static string from(std::vector<Player*> players,int solt)
	{
		return ((ItemStack&)(players[0]->getInventory().getItem(solt))).getNbt()->toSNBT();
	}
	static void to(string snbt, CommandOutput& output, string path)
	{
		asyncFileOut(path, snbt, std::ios_base::app);
		output.success("save in " + path);
	}
	static void to(string snbt, CommandOutput& output, std::vector<Actor*> actors)
	{
		int count = 0;
		auto nbt = CompoundTag::fromSNBT(snbt);
		for (auto actor : actors)
		{
			nbt->setActor(actor);
			count++;
		}
		output.success(fmt::to_string(count));
	}
	static void to(string snbt, CommandOutput& output, BlockPos pos, int dim)
	{
		auto nbt = CompoundTag::fromSNBT(snbt);
		nbt->setBlock(Global<Level>->getBlock(pos, dim));
	}
	static void to(string snbt, CommandOutput& output, std::vector<Player*> players, int solt)
	{
		int count = 0;
		for (auto player : players)
		{
			player->getInventory().setItem(solt, *ItemStack::create(CompoundTag::fromSNBT(snbt)));
			count++;
		}
		output.success(fmt::to_string(count));
	}
public:
	void execute(CommandOrigin const& ori, CommandOutput& output) const override
	{

		if (!ori.getPlayer()->isPlayer() || ori.getPlayer()->isOP())
		{
			string snbt;
			if (fromBlockIsSet)
			{
				from(fromBlock.getBlockPos(ori, Vec3()), ori.getDimension()->getDimensionId());
			}
			else if (fromActorIsSet)
			{
				auto results = fromActor.results(ori);
				snbt = from(*results.data);
			}
			else if (fromPathIsSet)
			{
				snbt = from(fromPath);
			}
			else if (fromJsonIsSet)
			{
				//snbt = from(fromJson);
			}
			else if (fromItemIsSet && fromSoltIsSet)
			{
				auto results = fromItem.results(ori);
				snbt = from(*results.data, fromSolt);
			}
			else if (fromItemIsSet)
			{
				auto results = fromItem.results(ori);
				snbt = from(*results.data, 0);
			}
			else
			{
				output.error("commands.stats.failed");
				return;
			}
			//fix crash
			if ((toActorIsSet || toBlockIsSet) && (fromActorIsSet && fromActor.results(ori).count() > 1))
			{
				output.error("commands.stats.failed");
				return;
			}
			else if (fromActorIsSet && snbt.size() > 2)
			{
				string temp;
				for (size_t i = 2; i < snbt.size() - 1; i++)
				{
					temp += snbt[i];
				}
				snbt = temp;
			}
			if (toActorIsSet && ori.getPermissionsLevel() == 4)
			{
				for (auto actor : toActor.results(ori))
				{
					if (actor->getUniqueID() == ori.getUUID())
					{
						output.error("commands.stats.failed");
						return;
					}
				}
			}

			if (toBlockIsSet)
			{
					to(snbt, output, toBlock.getBlockPos(ori, Vec3()), ori.getDimension()->getDimensionId());
			}
			else if (toActorIsSet)
			{
				auto results = fromActor.results(ori);
				to(snbt, output, *results.data);
			}
			else if (toPathIsSet)
			{
				to(snbt, output, toPath);
			}
			else if (toItemIsSet && toSoltIsSet)
			{
				auto results = fromItem.results(ori);
				to(snbt, output, *results.data, toSolt);
			}
			else if (toItemIsSet)
			{
				auto results = fromItem.results(ori);
				to(snbt, output, *results.data, 0);
			}
			else
			{
				output.success(snbt);
			}
			
		}
	}
	static void setup(CommandRegistry* registry)
	{
		using namespace RegisterCommandHelper;
		registry->registerCommand("nbt", "nbt", CommandPermissionLevel::Any, { (CommandFlagValue)0 }, { (CommandFlagValue)0x80 });
		//from actor
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromActor, "from", &NewNbtCmd::fromActorIsSet), makeOptional(&NewNbtCmd::toActor, "to", &NewNbtCmd::toActorIsSet));
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromActor, "from", &NewNbtCmd::fromActorIsSet), makeOptional(&NewNbtCmd::toBlock, "to", &NewNbtCmd::toBlockIsSet));
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromActor, "from", &NewNbtCmd::fromActorIsSet), makeOptional(&NewNbtCmd::toPath, "to", &NewNbtCmd::toPathIsSet));
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromActor, "from", &NewNbtCmd::fromActorIsSet), makeOptional(&NewNbtCmd::toSolt, "to", &NewNbtCmd::toSoltIsSet), makeOptional(&NewNbtCmd::toItem, "to", &NewNbtCmd::toItemIsSet));

		//from block
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromBlock, "from", &NewNbtCmd::fromBlockIsSet), makeOptional(&NewNbtCmd::toActor, "to", &NewNbtCmd::toActorIsSet));
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromBlock, "from", &NewNbtCmd::fromBlockIsSet), makeOptional(&NewNbtCmd::toBlock, "to", &NewNbtCmd::toBlockIsSet));
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromBlock, "from", &NewNbtCmd::fromBlockIsSet), makeOptional(&NewNbtCmd::toPath, "to", &NewNbtCmd::toPathIsSet));
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromBlock, "from", &NewNbtCmd::fromBlockIsSet), makeOptional(&NewNbtCmd::toSolt, "to", &NewNbtCmd::toSoltIsSet), makeOptional(&NewNbtCmd::toItem, "to", &NewNbtCmd::toItemIsSet));

		//from path
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromPath, "from", &NewNbtCmd::fromPathIsSet), makeOptional(&NewNbtCmd::toActor, "to", &NewNbtCmd::toActorIsSet));
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromPath, "from", &NewNbtCmd::fromPathIsSet), makeOptional(&NewNbtCmd::toBlock, "to", &NewNbtCmd::toBlockIsSet));
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromPath, "from", &NewNbtCmd::fromPathIsSet), makeOptional(&NewNbtCmd::toPath, "to", &NewNbtCmd::toPathIsSet));
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromPath, "from", &NewNbtCmd::fromPathIsSet), makeOptional(&NewNbtCmd::toSolt, "to", &NewNbtCmd::toSoltIsSet), makeOptional(&NewNbtCmd::toItem, "to", &NewNbtCmd::toItemIsSet));

		//from json
		//registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromJson, "from", &NewNbtCmd::fromJsonIsSet), makeOptional(&NewNbtCmd::toActor, "to", &NewNbtCmd::toActorIsSet));
		//registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromJson, "from", &NewNbtCmd::fromJsonIsSet), makeOptional(&NewNbtCmd::toBlock, "to", &NewNbtCmd::toBlockIsSet));
		//registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromJson, "from", &NewNbtCmd::fromJsonIsSet), makeOptional(&NewNbtCmd::toPath, "to", &NewNbtCmd::toPathIsSet));
		//registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromJson, "from", &NewNbtCmd::fromJsonIsSet), makeOptional(&NewNbtCmd::toItem, "to", &NewNbtCmd::toItemIsSet), makeOptional(&NewNbtCmd::toSolt, "to", &NewNbtCmd::toSoltIsSet));

		//from item
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromSolt, "from", &NewNbtCmd::fromSoltIsSet), makeOptional(&NewNbtCmd::fromItem, "from", &NewNbtCmd::fromItemIsSet), makeOptional(&NewNbtCmd::toActor, "to", &NewNbtCmd::toActorIsSet));
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromSolt, "from", &NewNbtCmd::fromSoltIsSet), makeOptional(&NewNbtCmd::fromItem, "from", &NewNbtCmd::fromItemIsSet), makeOptional(&NewNbtCmd::toBlock, "to", &NewNbtCmd::toBlockIsSet));
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromSolt, "from", &NewNbtCmd::fromSoltIsSet), makeOptional(&NewNbtCmd::fromItem, "from", &NewNbtCmd::fromItemIsSet), makeOptional(&NewNbtCmd::toPath, "to", &NewNbtCmd::toPathIsSet));
		registry->registerOverload<NewNbtCmd>("nbt", makeOptional(&NewNbtCmd::fromSolt, "from", &NewNbtCmd::fromSoltIsSet), makeOptional(&NewNbtCmd::fromItem, "from", &NewNbtCmd::fromItemIsSet), makeOptional(&NewNbtCmd::toSolt, "to", &NewNbtCmd::toSoltIsSet), makeOptional(&NewNbtCmd::toItem, "to", &NewNbtCmd::toItemIsSet));
	}
};

class SNbtSyntaxHighlightingCommand : public Command
{
	std::string syntaxHighlightingMode;
	bool syntaxHighlightingModeIsSet;
public:
	void execute(CommandOrigin const& ori, CommandOutput& output) const override
	{

	}
	static void setup(CommandRegistry* registry)
	{
		using namespace RegisterCommandHelper;
		registry->registerCommand("snbtsyntaxhighlighting", "snbt syntaxh highlighting", CommandPermissionLevel::Any, { (CommandFlagValue)0 }, { (CommandFlagValue)0x80 });
		registry->registerOverload<SNbtSyntaxHighlightingCommand>("snbtsyntaxhighlighting", makeOptional<SNbtSyntaxHighlightingCommand>(&SNbtSyntaxHighlightingCommand::syntaxHighlightingMode, "syntaxhighlightingmode", &SNbtSyntaxHighlightingCommand::syntaxHighlightingModeIsSet));
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
		NewNbtCmd::setup(regCmdEvent.mCommandRegistry);
		return true; 
		});
}
